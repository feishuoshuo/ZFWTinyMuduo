#pragma once

#include <functional> // function
#include <memory>     // weak_ptr
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
  private:
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_; // 事件循环
    const int fd_;    // fd, Poller监听的对象
    int events;       // 注册fd感兴趣的事件
    int revents_;     // poller返回的具体发生的事件
    int index_;

  public:
    typedef std::function<void()> EventCallback;              // 事件的回调
    typedef std::function<void(Timestamp)> ReadEventCallback; // 只读事件的回调
    // TAG：[编程好习惯] 由于这里定义的EventLoop的对象是指针4字节确定了，但是下面的Timestamp定义的对象大小位置，所以不好采用前置声明类
    Channel(EventLoop *loop, int fd);
    ~Channel();

    void handleEvent(Timestamp receiveTime);
  };

} // namespace zfwmuduo