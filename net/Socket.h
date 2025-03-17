#pragma once

#include "../base/noncopyable.h"
/**
 * Socket主要是对fd的封装
 */

namespace zfwmuduo
{
  class InetAddress;
  class Socket : noncopyable
  {
  public:
    explicit Socket(int sockfd) : sockfd_(sockfd) {};
    ~Socket();

    int fd() const { return sockfd_; }
    void bindAddress(const InetAddress &localaddr);
    void listen();
    int accept(InetAddress *peeraddr);

    void shutdownWrite();

    // 更改TCP选项相关操作
    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

  private:
    const int sockfd_;
  };

} // namespace zfwmuduo