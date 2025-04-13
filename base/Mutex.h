#pragma once

#include "noncopyable.h"
#include "CurrentThread.h"

#include <assert.h>
#include <pthread.h> // for pthread_mutex_t
/**
 * 互斥锁 mutex
 *
 * pthread_mutex_init():初始化互斥锁
 */

namespace zfwmuduo
{
  class MutexLock : noncopyable
  {
  public:
    MutexLock() : holder_(0) { pthread_mutex_init(&mutex_, nullptr); }
    ~MutexLock()
    {
      assert(holder_ == 0);
      pthread_mutex_destroy(&mutex_);
    }

    // 用于程序断言
    bool isLockedByThisThread() { return holder_ == currentThread::tid(); }
    void assertLocked() { assert(isLockedByThisThread()); }

    // internal usage

    void lock()
    {
      pthread_mutex_lock(&mutex_);
      holder_ = currentThread::tid();
    }
    // TAG: 下面顺序不能颠倒！否则会有线程安全/死锁等问题！细品！！
    void unlock()
    {
      holder_ = 0; // 设置当前所未被持有
      pthread_mutex_unlock(&mutex_);
    }

    pthread_mutex_t *getPthreadMutex() // non-const
    {
      return &mutex_;
    }

  private:
    // NOTE: 是 POSIX 线程库（pthread）中用于表示互斥锁（mutex）的一个数据类型。
    // 互斥锁是一种同步原语
    pthread_mutex_t mutex_;
    pid_t holder_; // 用于记录当前持有锁的线程ID（或进程ID）
  };

  // TAG: 使用RAII手法封装互斥锁的创建与销毁
  class MutexLockGuard : noncopyable
  {
  public:
    explicit MutexLockGuard(MutexLock &mutex) : mutex_(mutex) { mutex_.lock(); }
    ~MutexLockGuard() { mutex_.unlock(); }

  private:
    MutexLock &mutex_;
  };

} // namespace zfwmuduo