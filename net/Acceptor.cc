#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h> // close()
#include "Acceptor.h"
#include "../base/Logger.h" // LOG_FATAL, LOG_ERROR
#include "InetAddress.h"

namespace zfwmuduo
{ // 为防止与其他文件中变量名重名而产生的重复定义, 在这里将其定义为静态函数static
  static int createNonblocking()
  {
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sockfd < 0)
      LOG_FATAL("%s:%s:%d listen socket create errno:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
    return sockfd;
  }

  Acceptor::Acceptor(EventLoop *loop,
                     const InetAddress &listenAddr,
                     bool reuseport) : loop_(loop),
                                       acceptSocket_(createNonblocking()), // 1-创建socket套接字
                                       acceptChannel_(loop, acceptSocket_.fd()),
                                       listenning_(false)
  {
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr); // 2-bind绑定socket

    //  baseloop --> acceptChannel_(listenfd) -->
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
  }

  // listenfd有事件发生了, 就是有新用户连接了
  void Acceptor::handleRead()
  {
    InetAddress peerAddr; // 客户端的address, 客户端发起了连接
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0)
    { // TcpServer::start() Acceptor.listen() 有新用户的连接 要执行一个回调(connfd--> channel --> subloop)
      if (newConnectionCallback_)
      { // 轮询找到subloop, 唤醒, 分发当前的新客户端的cannel
        newConnectionCallback_(connfd, peerAddr);
      }
      else // 如果新客户端来了却没有相应的回调, 说明根本没有办法服务客户端, 因此直接close()
      {
        ::close(connfd);
      }
    }
    else // 出错
    {
      LOG_ERROR("%s:%s:%d accept errno:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
      if (errno == EMFILE)
      { // 警告调整当前进程文件描述符的上限
        LOG_ERROR("%s:%s:%d sockfd reached limit \n", __FILE__, __FUNCTION__, __LINE__);
      }
    }
  }

  Acceptor::~Acceptor()
  {
    acceptChannel_.disableAll();
    acceptChannel_.remove();
  }

  void Acceptor::listen()
  {
    listenning_ = true;
    acceptSocket_.listen();         // 3-listen监听
    acceptChannel_.enableReading(); // 将acceptChannel_注册到Poller中
  }
} // namespace zfwmuduo