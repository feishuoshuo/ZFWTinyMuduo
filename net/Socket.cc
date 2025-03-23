#include <unistd.h> // close()
#include <sys/types.h>
#include <sys/socket.h>  // 包含套接字相关定义
#include <netinet/tcp.h> // 包含 TCP 相关选项（如 TCP_NODELAY）
#include <netinet/in.h>  // 包含 IP 相关定义
#include <strings.h>     // bzero()
#include "Socket.h"      // LOG_FATAL, LOG_ERROR
#include "InetAddress.h"
#include "../base/Logger.h"

namespace zfwmuduo
{
  Socket::~Socket()
  {
    ::close(sockfd_);
  }

  void Socket::bindAddress(const InetAddress &localaddr)
  {
    // sizeof(sockaddr_in) 获取类型 sockaddr_in 的大小
    // 由于 sockaddr_in 和 sockaddr 的内存布局是兼容的，这种转换是安全的
    // TODO:reinterpret_cast<sockaddr *>(const_cast<sockaddr_in *>(localaddr.getSockAddr())) 我更改的
    //(sockaddr *)localaddr.getSockAddr() 原muduo
    if (0 != ::bind(sockfd_, reinterpret_cast<sockaddr *>(const_cast<sockaddr_in *>(localaddr.getSockAddr())), sizeof(sockaddr_in)))
    {
      LOG_FATAL("socket:: bind sockfd:%d fail\n", sockfd_);
    }
  }

  void Socket::listen()
  {
    if (0 != ::listen(sockfd_, 1024))
    {
      LOG_FATAL("socket:: listen sockfd:%d fail\n", sockfd_);
    }
  }

  int Socket::accept(InetAddress *peeraddr)
  {

    /**
     * 1. accept函数的参数不合法
     * 2. 对返回的connfd没有设置非阻塞
     * Reactor模型 one loop per thread
     * poller + non-blocking IO
     **/

    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    bzero(&addr, sizeof addr); // 将 addr 初始化为零
    // accept 函数通过返回 -1 和设置 errno 来表示失败
    int connfd = ::accept(sockfd_, (sockaddr *)&addr, &len);
    if (connfd >= 0)
    {
      peeraddr->setSockAddr(addr);
    }
    return connfd;
  }

  void Socket::shutdownWrite()
  {
    if (::shutdown(sockfd_, SHUT_WR) < 0)
      LOG_ERROR("socket::shuntdownWrite error!");
  }

  // 更改TCP选项相关操作
  void Socket::setTcpNoDelay(bool on) // 协议级别(IPPROTO_TCP)
  {
    int optval = on ? 1 : 0;
    // NOTE: setsockopt 是一个强大的系统调用，用于动态配置套接字的行为. 帮助更好地控制网络通信的细节
    //  NOTE: TCP_NODELAY 是一个 TCP 选项，用于禁用 Nagle 算法。
    //  Nagle 算法会将小的数据包合并成较大的数据包发送，以减少网络流量。
    //  禁用 Nagle 算法可以减少延迟，但可能会增加网络流量
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, static_cast<socklen_t>(sizeof optval));
  }

  void Socket::setReuseAddr(bool on) // socket级别(SQL_SOCKET)
  {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof optval));
  }

  void Socket::setReusePort(bool on) // socket级别(SQL_SOCKET)
  {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, static_cast<socklen_t>(sizeof optval));
  }

  void Socket::setKeepAlive(bool on) // socket级别(SQL_SOCKET)
  {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, static_cast<socklen_t>(sizeof optval));
  }
} // namespace zfwmuduo