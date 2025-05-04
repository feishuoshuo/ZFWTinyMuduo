#include "CountDownLatch.h"
namespace zfwmuduo
{
  CountDownLatch::CountDownLatch(int count) : mutex_(), condition_(mutex_), count_(count) {}

  void CountDownLatch::wait()
  {
    zfwmuduo::MutexLockGuard lock(mutex_);
    while (count_ > 0)
    {
      condition_.wait();
    }
  }

  void CountDownLatch::countDown()
  {
    zfwmuduo::MutexLockGuard lock(mutex_);
    --count_;
    if (count_ == 0)
    {
      condition_.notifyAll();
    }
  }

  int CountDownLatch::getCount() const
  {
    zfwmuduo::MutexLockGuard lock(mutex_);
    return count_;
  }
} // namespace zfwmuduo
