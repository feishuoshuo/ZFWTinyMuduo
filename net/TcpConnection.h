#pragma once

#include <memory> // enable_shared_from_this<T>
#include <string>
#include <atomic> // atomic_int
#include "../base/noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "../base/Timestamp.h"

/**
 * 主要负责连接
 * 主要表示已连接/通信成功的用户, 在服务端的一些数据的封装
 *
 * TcpServer -> Acceptor -> 有一个新用户连接, 通过accept函数拿到confd
 * -> TcpConnection 设置回调 -> Channel -> Poller -> Channel的回调操作
 */

namespace zfwmuduo
{
  class Channel;
  class EventLoop;
  class Socket;

  // NOTE:enable_shared_from_this<T> 作用是允许一个类的实例安全地生成一个指向自身的 std::shared_ptr
  /**
   * 在使用智能指针（如 std::shared_ptr）管理对象生命周期时，
   * 有时需要从对象内部获取一个指向自身的 std::shared_ptr。
   * 直接使用 this 指针是不安全的，因为 this 是一个裸指针，
   * 无法参与 std::shared_ptr 的引用计数机制。
   * 而 std::enable_shared_from_this 提供了一种安全的方式来实现这一点。
   */
  class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
  {
  public:
    TcpConnection(EventLoop *loop,
                  const std::string &name,
                  int sockfd,
                  const InetAddress &localAddr,
                  const InetAddress &peerAddr);
    ~TcpConnection();

    EventLoop *getLoop() const { return loop_; }
    const std::string &name() const { return name_; }
    const InetAddress &localAddress() const { return localAddr_; }
    const InetAddress &peerAddress() const { return peerAddr_; }

    bool connected() const { return state_ == kConnected; }
    bool disconnected() const { return state_ == kDisconnected; }

    void send(const std::string &buf); // 用于发送数据
    void shutdown();                   // 关闭连接

    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }
    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark)
    {
      highWaterMarkCallback_ = cb;
      highWaterMark_ = highWaterMark;
    }
    void setCloseCallback(const CloseCallback &cb) { closeCallback_ = cb; }

    void connectEstablished(); // 连接建立
    void connectDestroyed();   // 连接销毁

  private:
    enum StateE // 表示连接状态
    {
      kDisconnected,
      kConnecting,
      kConnected,
      kDisconnecting
    };
    void setState(StateE state) { state_ = state; }

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void *data, size_t len);
    void shutdownInLoop();

    EventLoop *loop_; // 这里绝对不是baseloop!! 因为TcpConnection都是在subloop里面管理的
    const std::string name_;
    std::atomic_int state_;
    bool reading_;

    // 和Acceptor类似  Acceptor在mainloop中; TcpConnection在subloop中
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_; // 本端的IP地址端口号
    const InetAddress peerAddr_;  // 对端的IP地址端口号

    // 用户设置的回调
    ConnectionCallback connectionCallback_;       // 有新连接时的回调
    MessageCallback messageCallback_;             // 有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_; // 消息发送后的回调
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;
    size_t highWaterMark_; // 水位标志

    Buffer inputBuffer_;  // 接收数据的缓冲区
    Buffer outputBuffer_; // 发送数据的缓冲区
  };

} // namespace zfwmuduo