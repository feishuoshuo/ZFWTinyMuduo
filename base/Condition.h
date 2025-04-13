#pragma once

#include "noncopyable.h"
#include "CurrentThread.h"
#include "Mutex.h"

#include <assert.h>
#include <pthread.h> // for pthread_mutex_t
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
    void notify() { pthread_cond_signal(&pcond_); }
    void notifyAll() { pthread_cond_broadcast(&pcond_); }

  private:
    zfwmuduo::MutexLock &mutex_;
    pthread_cond_t pcond_;
  };

} // namespace zfwmuduo