#pragma once
#include "../base/noncopyable.h"
#include "InetAddress.h"
#include "TimerId.h"
#include <memory>
#include <functional> // function

/**
 * Connector: 用于主动发起 TCP 连接的类。它负责建立连接，并在连接成功后通知应用程序
 */

namespace zfwmuduo
{
  class Channel;
  class EventLoop;

  class Connector : noncopyable
  {
  public:
    typedef std::function<void(int sockfd)> NewConnectionCallback;

    Connector(EventLoop *loop, const InetAddress &serverAddr);
    ~Connector();

    void setNewConnectionCallback(const NewConnectionCallback &cb) { newConnectionCallback_ = cb; }

    void start();   // can be called in any thread
    void restart(); // must be called in loop thread
    void stop();    // can be called in any thread
    void remove();

  private:
    enum States
    {
      kDisconnected,
      kConnecting,
      kConnected
    };
    static const int kInitRetryDelayMs = 500;      // 初始重连延迟时长
    static const int kMaxRetryDelayMs = 30 * 1000; // 最大的重连延迟时长

    void startInLoop();
    void connect();
    void setState(States s) { state_ = s; }

    EventLoop *loop_;
    InetAddress serverAddr_;
    // std::unique_ptr<Channel> channel_;
    NewConnectionCallback newConnectionCallback_;
    TimerId timerId_;  // 标识定时器的 ID
    bool connect_;     // atomic 是否尝试连接的状态标识
    States state_;     // 表示 Connector 的当前状态
    int retryDelayMs_; // 重连的延迟时间
  };
} // namespace zfwmuduo