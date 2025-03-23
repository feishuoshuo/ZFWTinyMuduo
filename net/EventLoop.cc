#include <sys/eventfd.h> // eventfd() EFD_NONBLOCK EFD_CLOEXEC
#include <errno.h>       // errno
#include <unistd.h>      //read()
#include <fcntl.h>
#include <memory> // unique_lock智能锁
#include <mutex>  // mutex
#include "EventLoop.h"
#include "Logger.h" // LOG_FATAL, LOG_DEBUG, LOG_ERROR, LOG_INFO
#include "Poller.h"
#include "Channel.h"
#include "../base/CurrentThread.h" // currentThread::tid()

namespace zfwmuduo
{
  // 防止一个线程创建多个EventLoop
  __thread EventLoop *t_loopInThisThread = nullptr; // 通过该全局变量指针控制每个线程只有一个EventLoop

  // 定义默认的Poller IO复用接口的超时时间
  const int kPollTimeMs = 10000;

  // 创建wakeupfd, 用来notify唤醒subReactor处理新来的channel
  int createEventfd()
  {
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC); // 用于通知睡眠的Eventloop唤醒处理对应的EventChannel
    if (evtfd < 0)
    {
      LOG_FATAL("eventfd error:%d\n", errno);
    }
    return evtfd;
  }

  EventLoop::EventLoop() : looping_(false),
                           quit_(false),
                           callingPendingFunctors_(false),
                           threadId_(zfwmuduo::currentThread::tid()),
                           poller_(Poller::newDefaultPoller(this)),
                           wakeupFd_(createEventfd()),
                           wakeupChannel_(new Channel(this, wakeupFd_))
  {
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
    if (t_loopInThisThread)
    {
      LOG_FATAL("Another EventLoop %p exists in this thread %d", t_loopInThisThread, threadId_);
    }
    else
    {
      t_loopInThisThread = this;
    }

    // TAG:基于事件驱动的网络编程框架中, 绑定器bind和函数对象，在C++中做事件通知基本都会使用到的
    // 设置wakeupfd的事件类型以及发生事件后的回调操作
    // &EventLoop::handleRead成员函数指针，指向handleRead方法
    // this当前对象的指针
    // 绑定后，当 wakeupfd 的可读事件发生时，会调用 this->handleRead() 方法。
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));

    // 每个EventLoop都将监听weakupchannel的EPOLLIN读事件
    wakeupChannel_->enableReading();
  }

  void EventLoop::handleRead()
  {
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one) // 读出现错误，程序还是允许继续运行LOG_ERROR
      LOG_ERROR("EventLoop::handleRead() reads %ld bytes instead of 8", n)
  }

  EventLoop::~EventLoop()
  {
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
  }

  // TAG：EventLoop核心！开启事件循环
  void EventLoop::loop()
  {
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this);

    // 轮询算法获取
    while (!quit_)
    {
      activateChannels_.clear();
      // 监听两类fd: 一种是client的fd[正常与客户端通信的]; 一种是wakeupfd[mainLoop与subLoop通信的手段]
      pollReturnTime_ = poller_->poll(kPollTimeMs, &activateChannels_); // 也是发生阻塞处, 需要被wakeup(相当于subLoop被wakeup)
      for (Channel *channel : activateChannels_)
      { // Poller监听哪些channel发生事件了, 然后上报给EventLoop, 通知channel处理相应的事件
        channel->handleEvent(pollReturnTime_);
      }
      // TAG: 执行当前EventLoop事件循环需要处理的回调操作
      /**
       * IO线程, 即mainLoop: accept(主要是接收新用户的连接), 之后会返回一个fd(我们会用channel去打包)
       * [已accept的channel会被分发给subLoop]
       * mainLoop事先会注册一个回调cb(需要subLoop来执行), wakeup subLoop后, 执行下面的方法(也就之前mainLoop注册的cb操作)
       */
      doPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping. \n", this);
    looping_ = false;
  }

  // TAG: 退出事件循环：1.loop在自己的线程中调用quit 2.在非loop的线程中, 调用loop的quit
  void EventLoop::quit()
  {
    quit_ = true;

    //!!! 如果是在其他线程中调用了quit. 例如, 在一个subLoop(workerThread)中, 调用了mainLoop(IO thread)的quit
    if (!isInLoopThread())
      wakeup();
  }

  // 执行回调
  void EventLoop::doPendingFunctors()
  {
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
      std::unique_lock<std::mutex> lock(mutex_);
      // TAG：[好手法学习]swap 相当于解放pendingFunctors_, 由于多线程下会不断有回调操作push进pendingFunctors_, 这就很容易导致它延长执行时间
      // 通过swap将当前pendingFunctors_容器中的所有回调操作转移到临时容器中进行操作. 这也不会妨碍到pendingFunctors_继续有新回调操作添加 是一种比较高效的手法
      functors.swap(pendingFunctors_);
    }

    for (const Functor &functor : functors)
      functor(); // 执行当前loop需要执行的回调操作

    callingPendingFunctors_ = false;
  }

  void EventLoop::runInLoop(Functor cb)
  {
    if (isInLoopThread())
    {
      cb(); // 在当前loop线程中，执行cb
    }
    else
    {
      queueInLoop(std::move(cb)); // 在非当前loop线程中执行cb, 就需要wakeup loop所在线程, 执行cb
    }
  }
  void EventLoop::queueInLoop(Functor cb)
  {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      pendingFunctors_.push_back(std::move(cb));
    }

    // wakeup相应的需要执行上面回调操作的loop的线程了
    // TAG: callingPendingFunctors_=true在这里表示:
    //[callingPendingFunctors_] 正在执行这个回调操作(doPendingFunctors), 没有阻塞在loop上, 这时又有新的回调操作添加(push_back(std::move(cb)))
    //[callingPendingFunctors_]当这个回调操作执行完之后, 又会循环回poll函数的地方阻塞起来，所以也需要wakeup
    if (!isInLoopThread() || callingPendingFunctors_)
      wakeup(); // 唤醒loop所在线程
  }

  // TAG: 唤醒loop所在线程  向wakeupfd_写一个数据, wakeupChannel就发生读事件, 当前loop线程就会被唤醒
  void EventLoop::wakeup()
  {
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
      LOG_ERROR("EventLoop::wakeup() writes %ld bytes instead of 8", n);
  }

  // EventLoop的方法 ==> Poller的方法
  void EventLoop::updateChannel(Channel *channel)
  {
    poller_->updateChannel(channel);
  }
  void EventLoop::removeChannel(Channel *channel)
  {
    poller_->removeChannel(channel);
  }
  bool EventLoop::hasChannel(Channel *channel)
  {
    return poller_->hasChannel(channel);
  }

} // namespace zfwmuduo