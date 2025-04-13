#pragma once

#include "../../base/Mutex.h"
#include "../../base/Condition.h"
#include "../../base/noncopyable.h"
#include "../../base/Thread.h"

/**
 * 条件变量 condition的实际案例之一
 */

namespace zfwmuduo
{
  class CountDownLatch : zfwmuduo::noncopyable
  {
  public:
    explicit CountDownLatch(int count) : mutex_(), condition_(mutex_), count_(count) {}

    void wait()
    {
      zfwmuduo::MutexLockGuard lock(mutex_);
      while (count_ > 0)
      {
        condition_.wait();
      }
    }
    void countDown()
    {
      zfwmuduo::MutexLockGuard lock(mutex_);
      --count_;
      if (count_ == 0)
      {
        condition_.notifyAll();
      }
    }

    int getCount() const
    {
      zfwmuduo::MutexLockGuard lock(mutex_);
      return count_;
    }

  private:
    // NOTE：mutable 让其即使在const成员函数中可以被修改
    mutable zfwmuduo::MutexLock mutex_;
    zfwmuduo::Condition condition_;
    int count_;
  };

} // namespace zfwmuduo