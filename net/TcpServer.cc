#include "TcpServer.h"
#include "TcpConnection.h"
#include "../base/Logger.h" // LOG_FATAL
#include <functional>       // bind()
#include <string>
#include <strings.h>    // bzero()
#include <sys/socket.h> // getsockname()

namespace zfwmuduo
{
  // 不接受用户传一个空指针给loop_
  static EventLoop *CheckLoopNotNull(EventLoop *loop)
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
                                        nextConnId_(1),
                                        started_(0)
  {
    // 当有新用户连接时，会执行TcpServer::newConnection回调
    acceptor_->setNewConnectionCallback(
        std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2)); // std::placeholders::_1是bind绑定器函数参数的占位符
  }

  TcpServer::~TcpServer()
  {
    LOG_INFO("TcpServer::~TcpServer [%s] destructing", name_.c_str());

    for (auto &item : connections_)
    {
      // TAG:这里就体现了智能指针的优势! 出了右括号, 它所指向的new出来的TcpConnection对象资源就自动释放了! 细品：为什么是map的结构
      TcpConnectionPtr conn(item.second);
      item.second.reset(); // reset()函数用于释放智能指针当前管理的资源

      // 销毁连接
      conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    }
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
      // TAG: &Acceptor::listen表示成员函数指针；acceptor_.get()表示对象指针[get()允许你访问底层的原始指针，而不会转移所有权]
      /**
       * 这个bind的作用等价于：
       * auto callable = [acceptor = acceptor_.get()]() {
       *   acceptor->listen();
       * };
       **/
      loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
  }

  // 有一个新的客户端的连接，acceptor会执行这个回调操作
  void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
  {
    // 1-根据轮询算法选择一个subloop, 来管理channel
    EventLoop *ioLoop = threadPool_->getNextLoop(); // 处理subloop
    char buf[64] = {0};
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_; // 因为(in mainloop)只有一个线程在处理它, 所以不涉及线程安全问题
    std::string connName = name_ + buf;

    LOG_INFO("TcpServer::newConnection [ %s ] - new connection [ %s ] from %s \n",
             name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());

    // 通过sockfd获取其绑定的本机ip地址和端口信息
    sockaddr_in local;
    ::bzero(&local, sizeof local); // 清零
    socklen_t addrlen = sizeof local;
    if (::getsockname(sockfd, (sockaddr *)&local, &addrlen) < 0)
    {
      LOG_ERROR("socket::getLocalAddr");
    }
    InetAddress localAddr(local);

    // 2-唤醒subloop; 根据连接成功的sockfd, 创建TcpConnection连接对象
    TcpConnectionPtr conn(new TcpConnection(ioLoop,
                                            connName,
                                            sockfd, // 通过这个fd 底层即可创建Socket对象和Channel
                                            localAddr,
                                            peerAddr));

    // 3 - 把当前confd封装成channel分发给subloop
    connections_[connName] = conn;
    // 下面的回调都是用户设置给TcpServer=>TcpConnection=>Channel=>Poller=>notify channel调用回调
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    // 设置了如何关闭连接的回调!!  conn->shutdown()
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));

    // 直接调用 TcpConnection::connectEstablished
    /**
     * 确保回调函数已设置：在调用 connectEstablished 之前，必须确保所有回调函数已经设置完毕，
     * 否则在 connectEstablished 中可能会触发未设置的回调函数，导致未定义行为。
     * 线程安全：runInLoop 会将任务提交到 ioLoop 的线程中执行，确保线程安全
     */
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
  }

  void TcpServer::removeConnection(const TcpConnectionPtr &conn)
  {
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
  }

  void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
  {
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s \n",
             name_.c_str(),
             conn->name().c_str());

    connections_.erase(conn->name());
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
  }

} // namespace zfwmuduo