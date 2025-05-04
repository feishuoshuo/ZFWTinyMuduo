#pragma once

#include <iostream> //int64_t
#include <string>

/**
 * 时间戳
 */

namespace zfwmuduo
{
  class Timestamp
  {
  private:
    int64_t microSecondsSinceEpoch_; // 从纪元开始的微秒数的时间戳

  public:
    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpoch); // explicit避免隐式转换
    static Timestamp now();
    static Timestamp invalid() { return Timestamp(); }
    std::string toString() const; // const只读方法

    bool valid() const { return microSecondsSinceEpoch_ > 0; }

    static const int kMicroSecondsPerSecond = 1000 * 1000;

    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
  };

  inline bool operator<(Timestamp lhs, Timestamp rhs)
  {
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
  }

  inline Timestamp addTime(Timestamp timestamp, double seconds)
  {
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
  }
} // namespace zfwmuduo