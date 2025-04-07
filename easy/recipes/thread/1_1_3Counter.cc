#include "../../../base/Mutex.h"
#include "../../../base/noncopyable.h"

#include <iostream> //int64_t

// A thread-safe counter
class Counter : zfwmuduo::noncopyable
{

public:
  friend void swap(Counter &a, Counter &b);

  Counter() : value_(0) {}
  Counter &operator=(const Counter &rhs)
  {
    if (this == &rhs)
      return *this;

    zfwmuduo::MutexLockGuard myLock(mutex_);
    zfwmuduo::MutexLockGuard itsLock(rhs.mutex_);
    value_ = rhs.value_;
    return *this;
  }

  int64_t value() const
  {
    zfwmuduo::MutexLockGuard lock(mutex_);
    return value_;
  }

  int64_t getAndIncrease()
  {
    zfwmuduo::MutexLockGuard lock(mutex_);
    // TAG：注意这两条语句不能合并在一起！！尤其是在多线程环境下！！
    int64_t ret = value_++;
    return ret;
  }

private:
  mutable zfwmuduo::MutexLock mutex_;
  int64_t value_;
};

void swap(Counter &a, Counter &b)
{
  zfwmuduo::MutexLockGuard aLock(a.mutex_); // potential dead lock
  zfwmuduo::MutexLockGuard bLock(b.mutex_);
  int64_t value = a.value_;
  a.value_ = b.value_;
  b.value_ = value;
}

int main()
{
  Counter c;
  std::cout << c.getAndIncrease() << std::endl;
  return 0;
}