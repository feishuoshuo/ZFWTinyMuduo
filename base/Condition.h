#pragma once

#include "noncopyable.h"
#include "CurrentThread.h"
#include "Mutex.h"

#include <assert.h>
#include <pthread.h> // for pthread_mutex_t
#include <errno.h>   // ETIMEDOUT用于表示超时错误
/**
 * 条件变量 condition
 */

namespace zfwmuduo
{
  class Condition : zfwmuduo::noncopyable
  {
  public:
    explicit Condition(zfwmuduo::MutexLock &mutex) : mutex_(mutex)
    {
      pthread_cond_init(&pcond_, nullptr);
    }
    ~Condition() { pthread_cond_destroy(&pcond_); }

    void wait() { pthread_cond_wait(&pcond_, mutex_.getPthreadMutex()); }
    // 指定等待时间
    bool waitForSeconds(int seconds)
    {
      // 获取当前时间
      struct timespec abstime;
      clock_gettime(CLOCK_REALTIME, &abstime);

      abstime.tv_sec += seconds; // 计算超时时间
      return ETIMEDOUT == pthread_cond_timedwait(&pcond_, mutex_.getPthreadMutex(), &abstime);
    }
    void notify() { pthread_cond_signal(&pcond_); }
    void notifyAll() { pthread_cond_broadcast(&pcond_); }

  private:
    zfwmuduo::MutexLock &mutex_;
    pthread_cond_t pcond_;
  };

} // namespace zfwmuduo