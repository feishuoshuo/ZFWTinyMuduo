#include "InetAddress.h"
#include <string.h>    // strlen()
#include <strings.h>   // bzero()
#include <arpa/inet.h> // inet_addr()、inet_ntop()

namespace zfwmuduo
{
  InetAddress::InetAddress(uint16_t port, std::string ip)
  {
    bzero(&addr_, sizeof addr_);  // 等价于源码的memZero() 赋0值清零 [bzero(s, n) 等价于 memset(s, 0, n)]
    addr_.sin_family = AF_INET;   // 设置Ipv4的协议族(AF_INET)
    addr_.sin_port = htons(port); //"host to network short"[主机字节序和网络字节序] 将本地字节序->网络字节序
    // NOTE：inet_addr()将用点分十进制字符串表示的Ipv4地址->网络字节序整数表示的Ipv4地址
    // NOTE：c_str()将std::string的内容转换为一个以空字符结尾的C风格字符串。它的返回值是一个 const char* 类型的指针，指向字符串的首字符。
    addr_.sin_addr.s_addr = inet_addr(ip.c_str()); // s_addr存放的是 网络字节序 Ipv4地址
  }

  // 获取IP信息
  std::string InetAddress::toIp() const
  { // addr_
    char buf[64] = {0};
    // NOTE：::inet_ntop()是一个用于将网络字节序的IP地址转换为点分十进制字符串的函数
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    return buf;
  }
  std::string InetAddress::toIpPort() const
  {
    // ip:port
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    size_t end = strlen(buf);
    uint16_t port = ntohs(addr_.sin_port); //"network to host short"[主机字节序和网络字节序] 将网络字节序->本地字节序
    sprintf(buf + end, ":%u", port);
    return buf;
  }
  uint16_t InetAddress::toPort() const
  { // port
    return ntohs(addr_.sin_port);
  }
} // namespace zfwmuduo

// #include <iostream>
// int main()
// {
//   zfwmuduo::InetAddress addr(8080);
//   std::cout << addr.toIpPort() << std::endl;
//   return 0;
// }