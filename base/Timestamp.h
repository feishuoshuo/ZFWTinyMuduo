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
    std::string toString() const; // const只读方法
  };

} // namespace zfwmuduo