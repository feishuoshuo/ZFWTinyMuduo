// #include <zfwmuduo/net/TcpServer.h>
// #include <zfwmuduo/base/Logger.h>
#include <string>
#include <functional> // bind()

#include "../net/TcpServer.h"
#include "../base/Logger.h"

class EchoServer
{
public:
  EchoServer(zfwmuduo::EventLoop *loop,
             const zfwmuduo::InetAddress &addr,
             const std::string &name) : server_(loop, name, addr), loop_(loop)
  {
    // 1-注册回调函数
    server_.setConnectionCallback(std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
    server_.setMessageCallback(std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    // 2-设置合适的loop线程数量 loopthread的数量
    server_.setThreadNum(3);
  }
  void start() { return server_.start(); }

private:
  // 连接建立或者断开的回调
  void onConnection(const zfwmuduo::TcpConnectionPtr &conn)
  {
    std::string condition;
    if (conn->connected())
    {
      condition = "UP";
    }
    else
    {
      condition = "DOWN";
    }
    // zfwmuduo::LOG_INFO("conn %s : %s", condition.c_str(), conn->peerAddress().toIpPort().c_str());
  }

  // 可读写事件回调
  void onMessage(const zfwmuduo::TcpConnectionPtr &conn, zfwmuduo::Buffer *buf, zfwmuduo::Timestamp time)
  {
    std::string msg = buf->retrieveAllAsString();
    conn->send(msg);
    conn->shutdown(); // 关闭写端 EPOLLHUP=> closeCallback_
  }

  zfwmuduo::EventLoop *loop_;
  zfwmuduo::TcpServer server_;
};

int main()
{
  zfwmuduo::EventLoop loop;
  zfwmuduo::InetAddress addr(8000);
  EchoServer server(&loop, addr, "EchoServer-01"); // Acceptor non-blocking listenfd create bind
  server.start();                                  // listen loopthread listenfd => acceptChannel => mainloop =>
  loop.loop();                                     // 启动mainloop的底层Poller

  return 0;
}