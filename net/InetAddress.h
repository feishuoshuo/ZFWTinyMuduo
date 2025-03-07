#pragma once

#include <string>
#include <netinet/in.h> // sockaddr_in

namespace zfwmuduo
{
  // 封装socket地址类型
  class InetAddress
  {
  private:
    sockaddr_in addr_; // sockaddr_in：创建一个IPv4 socket地址

  public:
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");
    explicit InetAddress(const sockaddr_in &addr) : addr_(addr) {}

    // 获取IP信息
    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const;

    const sockaddr_in *getSockAddr() const { return &addr_; } // 由于返回的是*指针 所以&取地址
  };

}