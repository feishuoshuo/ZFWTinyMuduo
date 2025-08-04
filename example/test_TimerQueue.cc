#include "../base/CurrentThread.h"
#include "../net/EventLoop.h"
#include "../net/TimerQueue.h"
#include "../base/Thread.h"
#include "../net/EventLoopThread.h"
#include <gtest/gtest.h>
#include <functional> // bind()

/**
 * 1-TimerQueue示例(使用runAfter、runEvery安排定时任务)
 */

class TimerQueueTest : public ::testing::Test
{
protected:
  zfwmuduo::EventLoop g_loop;
  std::atomic<int> cnt{0};

  void printTid()
  {
    printf("pid = %d, tid = %d\n", getpid(), zfwmuduo::currentThread::tid());
    printf("now %s\n", zfwmuduo::Timestamp::now().toString().c_str());
  }

  void print(const char *msg)
  {
    printf("msg %s %s\n", zfwmuduo::Timestamp::now().toString().c_str(), msg);
    if (++cnt == 20)
    {
      g_loop.quit();
    }
  }
};
//===================== 测试 Channel 的功能 =====================

TEST_F(TimerQueueTest, BasicTest)
{
  printTid();

  print("main");
  g_loop.runAfter(1, [this]()
                  { print("once1"); });
  g_loop.runAfter(1.5, [this]()
                  { print("once1.5"); });
  g_loop.runAfter(2.5, [this]()
                  { print("once2.5"); });
  g_loop.runAfter(3.5, [this]()
                  { print("once3.5"); });
  g_loop.runEvery(2, [this]()
                  { print("every2"); });
  g_loop.runEvery(3, [this]()
                  { print("every3"); });

  g_loop.loop();
  print("main loop exits");
  std::this_thread::sleep_for(std::chrono::seconds(1));
}