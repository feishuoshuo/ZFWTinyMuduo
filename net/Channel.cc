#include "Channel.h"
#include "EventLoop.h"
#include "../base/Logger.h" // LOG_INFO、
#include <sys/epoll.h>      // EPOLLN、EPOLLPRI

namespace zfwmuduo
{
  const int Channel::kNoneEvent = 0;                  // 无任何事件
  const int Channel::kReadEvent = EPOLLIN | EPOLLPRI; // 读事件
  const int Channel::kWriteEvent = EPOLLOUT;          // 写事件

  // EventLoop：ChannelList + Poller ("孩子之间无法直接访问, 需要通过父亲间接沟通")
  Channel::Channel(EventLoop *loop, int fd) : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false) {}

  Channel::~Channel()
  {
    // 是否在当前的事件循环所在的线程中, 去析构的Channel
    //   if (loop_->isInLoopThread())
    // {
    //   assert(!loop_->hasChannel(this));
    // }
  }

  // ======EventLoop("父")：ChannelList + Poller ("孩子之间无法直接访问, 需要通过父亲间接沟通")=======
  // 当改变Channel所表示fd的events事件后，update负责在poller里面更改fd相应的事件epoll_ctl
  void Channel::update()
  {
    // 通过Channel所属的EventLoop,调用Poller的相应方法，来注册fd的events事件
    loop_->updateChannel(this); // 当前对象this(即Channel)直接传入
  }

  // 在Channel所属的EventLoop中, 把当前的Channel删除掉
  void Channel::remove()
  {
    loop_->removeChannel(this);
  }

  // TODO：tie何时调用
  void Channel::tie(const std::shared_ptr<void> &obj)
  {
    tie_ = obj;
    tied_ = true; // 绑定过后标记true
  }

  void Channel::handleEvent(Timestamp receiveTime)
  {
    if (tied_) // 已于其他对象所绑定
    {
      std::shared_ptr<void> guard = tie_.lock(); // 尝试将 weak_ptr --> shared_ptr
      if (guard)
        handleEventWithGuard(receiveTime);
    }
    else
      handleEventWithGuard(receiveTime);
  }

  // 根据poller通知的channel发生的具体事件, 由channel来执行相应的回调操作
  void Channel::handleEventWithGuard(Timestamp receiveTime)
  {
    LOG_INFO("Channel handleEvent revents : %d", revents_);

    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
      if (closeCallback_)
        closeCallback_();
    }

    if (revents_ & EPOLLERR)
    {
      if (errorCallback_)
        errorCallback_();
    }

    if (revents_ & EPOLLIN)
    {
      if (readCallback_)
        readCallback_(receiveTime);
    }

    if (revents_ & EPOLLOUT)
    {
      if (writeCallback_)
        writeCallback_();
    }
  }
} // namespace zfwmuduo