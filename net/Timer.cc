#include "Timer.h"
#include "../base/Timestamp.h"

std::atomic<int64_t> zfwmuduo::Timer::s_numCrteated_(0);

namespace zfwmuduo
{
  void Timer::restart(zfwmuduo::Timestamp now)
  {
    if (repeat_)
    {
      expiration_ = zfwmuduo::addTime(now, interval_);
    }
    else
    {
      expiration_ = zfwmuduo::Timestamp::invalid();
    }
  }
} // namespace zfwmuduo