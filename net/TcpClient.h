#pragma once

#include <functional> // function
#include <string>
#include <memory> // unique_ptr, shared_ptr
#include <atomic> // AtomicInt
#include <unordered_map>

#include "../base/noncopyable.h"
#include "../base/Mutex.h"
#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "Callbacks.h"

namespace zfwmuduo
{
  class Connector;
  typedef std::shared_ptr<Connector> ConnectorPtr;

  class TcpClient : noncopyable
  {
  public:
    TcpClient(EventLoop *loop, const InetAddress &serverAddr, const std::string nameArg);
    ~TcpClient();

    void connect();
    void disconnect();
    // 停止客户端的连接尝试
    void stop();

    // 获取相关对象
    TcpConnectionPtr connection() const
    {
      // /TcpConnection 对象的生命周期和状态可能会在多线程环境中发生变化，而加锁是为了确保线程安全
      MutexLockGuard lock(mutex_);
      return connection_;
    }
    EventLoop *getLoop() const { return loop_; }
    const std::string &name() const { return name_; }

    // 设置连接回调callback 非线程安全
    void setConnectionCallback(ConnectionCallback cb) { connectionCallback_ = std::move(cb); }
    void setMessageCallback(MessageCallback cb) { messageCallback_ = std::move(cb); }
    void setWriteCompleteCallback(WriteCompleteCallback cb) { writeCompleteCallback_ = std::move(cb); }

  private:
    void newConnction(int sockfd); /* 非线程安全 但是在Loop中 */

    EventLoop *loop_;
    ConnectorPtr connector_;
    const std::string name_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;

    mutable MutexLock mutex_;
    TcpConnectionPtr connection_;
  };
} // namespace zfwmuduo