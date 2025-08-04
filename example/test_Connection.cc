#include "../base/CurrentThread.h"
#include "../net/EventLoop.h"
#include "../net/TimerQueue.h"
#include "../net/TcpServer.h"
#include "../base/Thread.h"
#include "../net/EventLoopThread.h"
#include "TcpClient.h"
#include <gtest/gtest.h>
#include <functional> // bind

/**
 * 1-discard服务
 * 9-echo服务
 * 10-发送两次数据，测试TcpConnection::send()
 * 11-chargen服务,使用WriteCompleteCallback
 */

class TcpServerTest : public ::testing::Test
{
protected:
  zfwmuduo::EventLoop loop;
  zfwmuduo::InetAddress listenAddr;
  zfwmuduo::TcpServer server;

  TcpServerTest() : listenAddr(9981), server(&loop, "server", listenAddr)
  {
    server.setConnectionCallback([this](const zfwmuduo::TcpConnectionPtr &conn)
                                 { this->onConnection(conn); });
    server.setMessageCallback([this](const zfwmuduo::TcpConnectionPtr &conn, const char *data, ssize_t len)
                              { this->onMessage(conn, data, len); });
    server.start();
    loop.loop();
  }

  void onConnection(const zfwmuduo::TcpConnectionPtr &conn)
  {
    if (conn->connected())
    {
      printf("onConnection(): new connection [%s] from %s\n",
             conn->name().c_str(),
             conn->peerAddress().toIpPort().c_str());
    }
    else
    {
      printf("onConnection(): connection [%s] is down\n",
             conn->name().c_str());
    }
  }

  void onMessage(const zfwmuduo::TcpConnectionPtr &conn,
                 const char *data,
                 ssize_t len)
  {
    printf("onMessage(): received %zd bytes from connection [%s]\n",
           len, conn->name().c_str());
  }
};
//===================== 测试 Channel 的功能 =====================
/**
 * 1-discard服务
 * 丢弃（discard）客户端发送的所有数据，不进行任何处理。
 * 这种服务通常用于测试和调试目的，例如验证网络连接是否正常建立和关闭，而不关心数据内容。
 */
TEST_F(TcpServerTest, TestDiscardService)
{
  std::atomic<bool> serverRunning{false};
  // 启动服务器
  server.start();
  serverRunning.store(true);

  // 等待服务器启动
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // 创建一个客户端连接
  zfwmuduo::InetAddress serverAddr(9981, "127.0.0.1");
  zfwmuduo::TcpClient client(&loop, serverAddr);
  client.connect();

  std::this_thread::sleep_for(std::chrono::seconds(1));
  client.send("Hello, server!");
  std::this_thread::sleep_for(std::chrono::seconds(1));
  client.disconnect();
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // 停止服务器
  serverRunning.store(false);
  loop.quit();
}