#pragma once
#include "noncopyable.h"
#include "Mutex.h" // MutexLock
#include <string>
#include <memory>
#include <fstream>
#include <atomic>

/**
 * 主要用于异步日志的日志文件的写入、刷新和滚动roll
 *
 * 仅为线程安全下使用
 * 直接使用标准库中的文件操作函数来实现日志文件的写入、刷新和滚动 std::ofstream
 */
namespace zfwmuduo
{
  class LogFile : noncopyable
  {
  private:
    std::unique_ptr<zfwmuduo::MutexLock> mutex_;

    const std::string basename_;
    const off_t rollSize_;

    time_t lastRoll_;
    time_t lastFlush_;
    std::ofstream file_;
    std::atomic<size_t> writtenBytes_;

    const static int kRollPerSeconds_ = 60 * 60 * 24; // 每个日志文件的时间周期（例如，每天滚动一次，kRollPerSeconds_ 为 24 小时的秒数）

  public:
    LogFile(std::string bname, off_t rollSize);
    ~LogFile();

    bool rollFile(bool addtime);
    void append(const char *logline, int len);
    void flush();
  };

} // namespace zfwmuduo