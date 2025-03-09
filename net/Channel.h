#pragma once

#include <functional> // function
#include <memory>     // weak_ptr shared_ptr
#include "../base/noncopyable.h"
#include "../base/Timestamp.h" //Timestamp
/**
 * Channel：通道, 封装了sockfd和其相关的event，如EPOLLIN、EPOLLOUT事件
 * 相当于epoll_wait(事件循环、事件分发器)
 */

namespace zfwmuduo
{
  class EventLoop; // 类的前置声明，可防止暴露过多信息给客户

  class Channel : noncopyable
  {
  public:
    typedef std::function<void()> EventCallback;              // 事件的回调
    typedef std::function<void(Timestamp)> ReadEventCallback; // 只读事件的回调
    // TAG：[编程好习惯] 由于这里定义的EventLoop的对象是指针4字节确定了，但是下面的Timestamp定义的对象大小位置，所以不好采用前置声明类
    Channel(EventLoop *loop, int fd);
    ~Channel();

    // fd得到poller通知后, 处理事件
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数对象
    // TAG: [编程好习惯] 这里涉及的对象可能会占用很大资源，因此采用移动语义move()"搬运"
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    // NOTE：防止当channel被手动remove掉，channel还在执行回调操作
    void tie(const std::shared_ptr<void> &);

    int fd() const { return fd_; }
    int events() const { return events_; }
    void set_revents(int revt) { revents_ = revt; } // used by pollers

    // 设置fd相应的事件状态
    void enableReading()
    {
      events_ |= kReadEvent; // 按位或 |=
      update();              /*调用epoll_trl, 将fd感兴趣的事件添加到epoll中 */
    }
    void disableReading()
    {
      events_ &= ~kReadEvent; // 按位与 &= 取反~
      update();
    }
    void enableWriting()
    {
      events_ |= kWriteEvent;
      update();
    }
    void disableWriting()
    {
      events_ &= ~kWriteEvent;
      update();
    }
    void disableAll()
    {
      events_ = kNoneEvent;
      update();
    }

    // 返回fd当前的事件状态
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    // for Poller 状态index_的设置
    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    // one loop per thread
    EventLoop *ownerLoop() { return loop_; }
    void remove();

  private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime); // 受保护的处理事件

    static const int kNoneEvent;  // 无任何事件
    static const int kReadEvent;  // 读事件
    static const int kWriteEvent; // 写事件

    EventLoop *loop_; // 事件循环
    const int fd_;    // fd, Poller监听的对象
    int events_;      // 注册fd感兴趣的事件
    int revents_;     // poller返回的具体发生的事件
    int index_;

    // NOTE：weak_ptr 解决循环引用问题
    std::weak_ptr<void> tie_;
    bool tied_; // 用于标记 Channel 是否已经与其他对象绑定或关联

    // 因为channel中能获取fd最终发生的具体事件revent,所以它负责调用具体事件的回调操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
  };

} // namespace zfwmuduo