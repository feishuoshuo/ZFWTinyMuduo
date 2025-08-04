#pragma once

#include <cstdint> // int64_t
#include "../base/copyable.h"

/**
 * 一个不透明的标识符，用于取消定时器
 */
namespace zfwmuduo
{
  class Timer;

  class TimerId : zfwmuduo::copyable
  {
  public:
    friend class TimerQueue;

    TimerId() : timer_(nullptr), sequence_(0) {}
    TimerId(Timer *timer, int64_t seq) : timer_(timer), sequence_(seq) {}

    bool valid() const { return timer_ != nullptr && sequence_ != 0; }

    friend class TimerQueue;

  private:
    Timer *timer_;
    int64_t sequence_;
  };
} // namespace zfwmuduo