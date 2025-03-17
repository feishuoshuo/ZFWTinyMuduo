#pragma once
/**
 * 由于用户主要是通过TcpServer使用muduo库相关功能，
 * 为了使用户使用避免自己额外包含相关头文件，
 * 因此均提前#include，而不用实现声明了
 */
#include <functional> // function
#include <string>
#include <memory> // unique_ptr, shared_ptr
#include <atomic> // AtomicInt
#include <unordered_map>
#include "noncopyable.h"
#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "Callbacks.h" //ConnectionCallback, MessageCallback, WriteCompleteCallback, ThreadInitCallback

/**
 * TcpServer: 对外服务器编程使用的类
 */

namespace zfwmuduo
{
  class TcpServer : noncopyable
  {
  public:
    // NOTE: std::function 可以封装多种类型的可调用对象，只要它们的调用签名匹配
    // void(EventLoop *)是一个函数签名，表示：
    // - 函数返回类型为 void（即不返回任何值）。
    // - 函数接受一个参数，类型为 EventLoop *（即指向 EventLoop 类型的指针）。
    typedef std::function<void(EventLoop *)> ThreadInitCallback;

    enum Option // 枚举: 选项, 是否对端口可重用
    {
      kNoReusePort, // 不重用端口
      kReusePort,
    };

    TcpServer(EventLoop *, std::string nameArg, const InetAddress &listenAddr, Option option = kNoReusePort);
    ~TcpServer();

    // 对外提供的查询接口
    const std::string &ipPort() const { return ipPort_; }
    const std::string name() const { return name_; }
    EventLoop *getLoop() const { return loop_; }

    // 设置线程初始化回调
    void setThreadInitCallback(const ThreadInitCallback &cb) { threadInitCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
    void setWriteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

    // 设置底层subloop的个数
    void setThreadNum(int numThreads);

    // 开启服务器监听
    void start();

  private:
    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    typedef std::unordered_map<std::string, TcpConnectionPtr> ConnectionMap;

    EventLoop *loop_; // baseloop 用户定义的loop

    const std::string ipPort_;
    const std::string name_;

    std::unique_ptr<Acceptor> acceptor_; // 运行在mainloop, 任务：监听新连接事件 | "avoid revealing Acceptor"

    std::shared_ptr<EventLoopThreadPool> threadPool_; // one loop per thread

    // 用户设置的回调
    ConnectionCallback connectionCallback_;       // 有新连接时的回调
    MessageCallback messageCallback_;             // 有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_; // 消息发送后的回调

    ThreadInitCallback threadInitCallback_; // 线程初始化的回调

    std::atomic_int started_;

    int nextConnId_;
    ConnectionMap connections_; // 保存所有的连接
  };

} // namespace zfwmuduo