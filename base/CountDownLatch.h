#pragma once

#include "Mutex.h"
#include "Condition.h"
#include "noncopyable.h"
#include "Thread.h"

/**
 * 条件变量 condition的实际案例之一:计数信号量
 *
 * 通过一个计数器来控制线程的同步，当计数器的值达到某个条件（通常是0）时，等待的线程会被释放
 *
 * 一个同步辅助工具，用于在多线程环境中协调线程的启动和完成。
 * 它允许一个或多个线程等待其他线程完成一系列操作后再继续执行
 */

namespace zfwmuduo
{
  class CountDownLatch : zfwmuduo::noncopyable
  {
  public:
    explicit CountDownLatch(int count);

    // 主线程等待计数信号量的计数器变为0
    void wait();

    // 工作线程完成任务后，调用countDown()减少计数器的值
    void countDown();

    int getCount() const;

  private:
    // NOTE：mutable 让其即使在const成员函数中可以被修改
    mutable zfwmuduo::MutexLock mutex_;
    zfwmuduo::Condition condition_;
    int count_;
  };

} // namespace zfwmuduo