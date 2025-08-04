#include "../base/CurrentThread.h"
#include "../net/EventLoop.h"
#include "../net/TimerQueue.h"
#include "../base/Thread.h"
#include "../net/EventLoopThread.h"
#include <gtest/gtest.h>
#include <sys/timerfd.h> // timerfd_settime()

/**
 * 1-用Channel关注timerfd的可读事件
 */

class TimerfdTest : public ::testing::Test
{
protected:
  zfwmuduo::EventLoop g_loop;
  std::atomic<bool> timerFired{false};

  void timeout()
  {
    printf("Timeout!\n");
    timerFired.store(true);
    g_loop.quit();
  }
};

//===================== 测试 Channel 的功能 =====================
/**
 * 1-用Channel关注timerfd的可读事件
 * 用timerfd实现了一个单次触发的定时器
 * 利用channel将timerfd的readable事件转发给timerout()函数
 */
// ASSERT_GE 一个断言宏，用于检查一个值是否大于或等于另一个值
TEST_F(TimerfdTest, BasicTest)
{
  printf("main(): pid = %d, tid = %d\n",
         getpid(), zfwmuduo::currentThread::tid());

  /**
   * CLOCK_MONOTONIC:使用单调始终,不受系统时间调整影响
   * TFD_NONBLOCK:设置为非阻塞模式fd
   * TFD_CLOEXEC:设置在执行exec()系统调用时,关闭该文件描述符
   */
  int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  ASSERT_GE(timerfd, 0) << "timerfd_create";

  zfwmuduo::Channel channel(&g_loop, timerfd);
  channel.setReadCallback([this](zfwmuduo::Timestamp t)
                          { this->timeout(); });
  channel.enableReading();

  /**
   * 设置定时器
   * it_value：表示定时器首次触发的时间
   * it_interval：表示定时器的周期性触发时间（这里设置为 0，表示单次触发）
   * tv_sec：秒
   */
  struct itimerspec howlong;
  memset(&howlong, 0, sizeof howlong); // 比bzero更优
  howlong.it_value.tv_sec = 5;

  int ret = ::timerfd_settime(timerfd, 0, &howlong, nullptr); // 设置timerfd定时器
  ASSERT_GE(ret, 0) << "timerfd_settime";

  g_loop.loop();
  ASSERT_TRUE(timerFired.load()) << "Timer should have fired";
  ::close(timerfd);
}