#include "Timestamp.h"
#include <time.h> //time localtime
#include <sys/time.h>

namespace zfwmuduo
{
  Timestamp::Timestamp() : microSecondsSinceEpoch_(0) {}
  Timestamp::Timestamp(int64_t microSecondsSinceEpoch) : microSecondsSinceEpoch_(microSecondsSinceEpoch) {}
  Timestamp Timestamp::now()
  {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t seconds = tv.tv_sec;
    return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
  }
  std::string Timestamp::toString() const
  {
    char buf[128] = {0};
    tm *tm_time = localtime(&microSecondsSinceEpoch_);
    // NOTE: snprintf 是一个更安全的函数(主要目的是避免缓冲区溢出)，用于将格式化的字符串写入一个指定大小的缓冲区
    snprintf(buf, 128, "%04d-%02d-%02d %02d:%02d:%02d",
             tm_time->tm_year + 1900,
             tm_time->tm_mon + 1,
             tm_time->tm_mday,
             tm_time->tm_hour,
             tm_time->tm_min,
             tm_time->tm_sec);
    return buf;
  }

} // namespace zfwmuduo

// #include <iostream>
// int main()
// {
//   std::cout << zfwmuduo::Timestamp::now().toString() << std::endl;
//   return 0;
// }