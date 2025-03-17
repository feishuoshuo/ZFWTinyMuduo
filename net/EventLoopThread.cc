#include "EventLoopThread.h"
#include "EventLoop.h"

namespace zfwmuduo
{
  EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
                                   const std::string &name) : loop_(nullptr),
                                                              exiting_(false),
                                                              thread_(std::bind(&EventLoopThread::threadFunc, this), name),
                                                              callback_(cb),
                                                              mutex_(), // 默认构造
                                                              cond_()   // 默认构造
  {
  }
  EventLoopThread::~EventLoopThread()
  {
    exiting_ = true;
    if (loop_)
    {
      loop_->quit();  // 将线程绑定的事件循环也退出
      thread_.join(); // 等待其相应的子线程结束
    }
  }

  // 启动循环
  EventLoop *EventLoopThread::startLoop()
  {
    thread_.start(); // 启动底层的新线程

    EventLoop *loop = nullptr;

    {
      std::unique_lock<std::mutex> lock(mutex_);
      while (!loop_)
      {
        cond_.wait(lock);
      }
      loop = loop_;
    }
    return loop;
  }

  // 下面是在单独的新线程中运行
  void EventLoopThread::threadFunc()
  {
    // 它是在线程函数的线程栈上创建的, 无需手动析构
    EventLoop loop; // 创建一个独立的eventloop, 和上面的线程是一一对应的 “one loop per thread!”

    if (callback_)
    {
      callback_(&loop);
    }

    {
      std::unique_lock<std::mutex> lock(mutex_);
      loop_ = &loop;
      cond_.notify_one(); // 条件变量 通知
    }

    loop.loop(); // EventLoop loop => Poller.poll

    // TAG：用锁对下面的操作进行了保护
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
  }

} // namespace zfwmuduo