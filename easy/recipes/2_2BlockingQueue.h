#pragma once

#include "../../base/Mutex.h"
#include "../../base/Condition.h"
#include "../../base/noncopyable.h"
#include "../../base/Thread.h"

#include <assert.h>
#include <deque>
/**
 * 条件变量 condition的实际案例之一
 * 无界阻塞队列
 */

namespace zfwmuduo
{
  template <typename T>
  class BlockingQueue : zfwmuduo::noncopyable
  {
  public:
    BlockingQueue() : mutex_(), notEmpty_(), queue_() {}

    void enqueue(const T &x)
    {
      zfwmuduo::MutexLockGuard lock(mutex_);
      queue_.push_back(x);
      notEmpty_.notify();
    }
    T dequeue()
    {
      zfwmuduo::MutexLockGuard lock(mutex_);
      // TAG：由于可能出现虚假唤醒，因此始终使用 while 循环
      // 必须用循环; 必须在判断之后再wait()
      /**
       * 虚假唤醒是指线程可能在没有收到通知的情况下被唤醒。
       * 为了避免这种情况导致的问题，使用 while 循环来检查条件。
       * 即使线程被虚假唤醒，它也会重新检查条件，并在条件不满足时继续等待。
       */
      while (queue_.empty())
      {
        // 这一步会原子地 unlock mutex并进入等待, 不会与enqueue死锁
        notEmpty_.wait();
      }
      assert(!queue_.empty());
      T front(queue_.front());
      queue_.pop_front();
      return front;
    }

  private:
    // TAG: 顺序很重要, 先mutex, 后condition！！
    mutable zfwmuduo::MutexLock mutex_;
    zfwmuduo::Condition notEmpty_;
    std::deque<T> queue_;
  };

} // namespace zfwmuduo