#include "TcpServer.h"
#include "TcpConnection.h"
#include "../base/Logger.h" // LOG_FATAL
#include <functional>       // bind()
#include <string>

namespace zfwmuduo
{
  // 不接受用户传一个空指针给loop_
  EventLoop *CheckLoopNotNull(EventLoop *loop)
  {
    if (!loop)
    {
      LOG_FATAL("%s:%s:%d mainloop is null! \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
  }

  TcpServer::TcpServer(EventLoop *loop,
                       std::string nameArg,
                       const InetAddress &listenAddr,
                       Option option) : loop_(CheckLoopNotNull(loop)),
                                        ipPort_(listenAddr.toIpPort()),
                                        name_(nameArg),
                                        acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)), // 需要监听新用户连接(listenAddr), loop相当于mainloop
                                        threadPool_(new EventLoopThreadPool(loop, nameArg)),             // 线程池对象创建，默认不会自己先开启额外线程(即刚开始只有主线程(它运行mainloop))
                                        connectionCallback_(),
                                        messageCallback_(),
                                        nextConnId_(1)
  {
    // 当有新用户连接时，会执行TcpServer::newConnection回调
    acceptor_->setNewConnectionCallback(
        std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2)); // std::placeholders::_1是bind绑定器函数参数的占位符
  }

  TcpServer::~TcpServer()
  {
  }

  void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
  {                                                 // 1-根据轮询算法选择一个subloop
    EventLoop *ioLoop = threadPool_->getNextLoop(); // 处理subloop
    char buf[64];
    snprintf(buf, sizeof buf, "%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;
    LOG_INFO("TcpServer::newConnection [ %s ] - new connection [ %s ] from %d", name_, connName, peerAddr.toIpPort());
    InetAddress localAddr();

    TcpConnectionPtr conn(new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));

    // 2-唤醒subloop

    // 3-把当前confd封装成channel分发给subloop
  }

  // 设置底层subloop的个数
  void TcpServer::setThreadNum(int numThreads)
  {
    threadPool_->setThreadNum(numThreads);
  }

  // 开启服务器监听  loop.loop()
  void TcpServer::start()
  {
    // TAG：started_++==0 防止一个TcpServer对象被start多次
    if (started_++ == 0)
    {
      threadPool_->start(threadInitCallback_); // 启动底层loop的线程池
      // TAG: &Acceptor::listen表示成员函数指针；acceptor_.get()表示对象指针
      /**
       * 这个bind的作用等价于：
       * auto callable = [acceptor = acceptor_.get()]() {
       *   acceptor->listen();
       * };
       **/
      loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
  }

  void TcpServer::removeConnection(const TcpConnectionPtr &conn)
  {
  }

  void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
  {
  }

} // namespace zfwmuduo