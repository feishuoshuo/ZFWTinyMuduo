#include "../base/CurrentThread.h"
#include "../net/EventLoop.h"
#include "../net/TimerQueue.h"
#include "../base/Thread.h"
#include "../net/EventLoopThread.h"
#include "../net/InetAddress.h"
#include "../net/Connector.h"
#include <gtest/gtest.h>

/**
 * 1-Connector示例
 */

class ConnectorTest : public ::testing::Test
{
protected:
  zfwmuduo::EventLoop *g_loop = new zfwmuduo::EventLoop();
  std::atomic<bool> timerFired{false};

  void connectCallback(int sockfd)
  {
    printf("connected.\n");
    timerFired.store(true);
    g_loop->quit();
  }
};

//===================== 测试 Channel 的功能 =====================
TEST_F(ConnectorTest, BasicTest)
{
  zfwmuduo::InetAddress addr(9981, "127.0.0.1");
  zfwmuduo::ConnectorPtr connector(new zfwmuduo::Connector(g_loop, addr));
  connector->setNewConnectionCallback([=](int sockfd)
                                      { connectCallback(sockfd); });
  connector->start();
  g_loop->loop();
}