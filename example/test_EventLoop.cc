#include "../base/CurrentThread.h"
#include "../net/EventLoop.h"
#include "../net/TimerQueue.h"
#include "../base/Thread.h"
#include "../net/EventLoopThread.h"
#include <gtest/gtest.h>
#include <atomic>
#include <stdio.h>
#include <memory>
#include <functional> // bind()
/**
 * 1-在两个线程里各自运行一个EventLoop
 * 2-IO线程调用EventLoop::runInLoop()、EventLoop::runAfter
 * 3-跨线程调用EventLoop::runInLoop()、EventLoop::runAfter
 */
void runInThread(const std::string &name)
{
  printf("%s(): pid = %d, tid = %d\n", name.c_str(),
         getpid(), zfwmuduo::currentThread::tid());
}

void threadFunc()
{
  runInThread("threadFunc");

  // 子线程运行 EventLoop
  zfwmuduo::EventLoop loop;
  loop.runAfter(3, [&loop]()
                {
                  printf("threadFunc() %d:Exiting EventLoop after 3 seconds.\n", zfwmuduo::currentThread::tid());
                  loop.quit(); // 退出 EventLoop
                });
  loop.loop();
  printf("threadFunc() %d: EventLoop exited.\n", zfwmuduo::currentThread::tid());
}

class RunAfter : public ::testing::Test
{
protected:
  zfwmuduo::EventLoop loop;
  std::atomic<int> flag{0};

  void run4()
  {
    printf("run4(): pid = %d, flag = %d\n", getpid(), flag.load());
    loop.quit(); // 先
  }

  void run3()
  {
    printf("run3(): pid = %d, flag = %d\n", getpid(), flag.load());
    loop.runAfter(1, [this]()
                  { this->run4(); }); // 使用 lambda 表达式捕获 this
    flag.store(3);                    // 后 还没来得及就quit了
  }

  void run2()
  {
    printf("run2(): pid = %d, flag = %d\n", getpid(), flag.load());
    loop.runInLoop([this]()
                   { this->run3(); });
  }

  void run1()
  {
    flag.store(1);
    printf("run1(): pid = %d, flag = %d\n", getpid(), flag.load());
    loop.runInLoop([this]()
                   { this->run2(); });
    flag.store(2);
  }
};

//===================== 测试 EventLoop 的功能 =====================
/**
 * 1-在两个线程里各自运行一个EventLoop
 * 主线程有EventLoop 实例, 其生成的各自子线程也有各自对应的EventLoop 实例
 * 每个 EventLoop 都在其所属线程中处理IO事件，因此不会导致程序崩溃
 */
TEST(EventLoopTest, BasicTest)
{
  runInThread("main");

  // 原子布尔变量,用于跟踪子线程中EventLoop是否完成
  std::atomic<bool> threadLoopFinished(false);
  auto threadFuncWrapper = [&]()
  {
    threadFunc();
    threadLoopFinished.store(true);
  };

  // 创建一个线程，运行 threadFunc
  zfwmuduo::Thread thread(threadFuncWrapper);
  thread.start();

  // 主线程运行 EventLoop
  zfwmuduo::EventLoop loop;
  loop.runAfter(5, [&loop]()
                {
                  printf("main() %d:Exiting EventLoop after 5 seconds.\n", zfwmuduo::currentThread::tid());
                  loop.quit(); // 退出 EventLoop
                });
  loop.loop();
  printf("main() %d: EventLoop exited.\n", zfwmuduo::currentThread::tid());

  thread.join();
  EXPECT_TRUE(threadLoopFinished.load());
  printf("main() %d: Test finished.\n", zfwmuduo::currentThread::tid());
}

// 2-IO线程调用EventLoop::runInLoop()、EventLoop::runAfter
TEST_F(RunAfter, TestRunAfter)
{
  printf("main(): pid = %d, flag = %d\n", getpid(), flag.load());

  // 在 EventLoop 中注册 run1 函数，延迟 2 秒执行
  loop.runAfter(2, [this]()
                { this->run1(); });

  // 启动 EventLoop
  loop.loop();

  // 验证最终的 flag 值是否为 3
  EXPECT_EQ(flag.load(), 2);
  printf("main(): pid = %d, flag = %d\n", getpid(), flag.load());
}

// 3-跨线程调用EventLoop::runInLoop()、EventLoop::runAfter
TEST(EventLoopTest, TestRunAfterCross)
{
  runInThread("main");

  zfwmuduo::EventLoopThread loopThread;
  zfwmuduo::EventLoop *loop = loopThread.startLoop();
  loop->runInLoop(std::bind(&runInThread, "runInThread"));
  sleep(1);
  loop->runAfter(2, std::bind(&runInThread, "runInThread"));
  sleep(3);
  loop->quit();

  printf("exit main().\n");
}