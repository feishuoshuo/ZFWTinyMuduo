#pragma once

#include <functional> // function
#include "../base/noncopyable.h"
#include "Socket.h"
#include "Channel.h"

/**
 * Acceptor
 * 1-从监听队列中监听新用户的连接 listenfd (主要就是对listenfd的封装)
 */

namespace zfwmuduo
{
  class EventLoop;
  class InetAddress;
  class Acceptor : noncopyable
  {
  public:
    typedef std::function<void(int sockfd, const InetAddress &)> NewConnectionCallback;

    Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
    ~Acceptor();

    // TODO：将其改成右值引用move转移对象资源
    // void setNewConnectionCallback(const NewConnectionCallback &cb) { newConnectionCallback_ = cb; }
    void setNewConnectionCallback(NewConnectionCallback &&cb) { newConnectionCallback_ = std::move(cb); }

    bool listenning() const { return listenning_; }
    void listen();

  private:
    void handleRead();

    EventLoop *loop_;       // 相当于mainloop/baseloop
    Socket acceptSocket_;   // listenfd的封装
    Channel acceptChannel_; // 提供相关Poller操作
    NewConnectionCallback newConnectionCallback_;
    bool listenning_;
  };

} // namespace zfwmuduo