#include "LogFile.h"
#include "Mutex.h" // MutexLock
#include <string>
#include <ctime>

namespace zfwmuduo
{
  LogFile::LogFile(std::string bname,
                   off_t rollSize) : basename_(bname),
                                     rollSize_(rollSize),
                                     lastRoll_(0),
                                     lastFlush_(0),
                                     writtenBytes_(0),
                                     mutex_(new zfwmuduo::MutexLock)
  {
    rollFile(false);
  }

  LogFile::~LogFile()
  {
    file_.close();
  }

  bool LogFile::rollFile(bool addtime)
  {
    std::string filename;
    time_t now = std::time(nullptr);
    if (addtime)
    {

      const struct tm *tm_time = localtime(&now);
      // 格式化日期和时间
      char timeBuffer[32];
      strftime(timeBuffer, sizeof(timeBuffer), "%Y%m%d", tm_time);
      filename = basename_ + "-" + timeBuffer + ".log";
    }
    else
      filename = basename_ + ".log";

    time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;

    if (now > lastRoll_) // 检查是否需要滚动日志
    {
      lastRoll_ = now;
      lastFlush_ = now;
      writtenBytes_ = 0;

      file_.close();
      file_.open(filename, std::ios::app); // std::ios::app文件打开模式为追加

      return true;
    }
    return false;
  }

  void LogFile::append(const char *logline, int len)
  {
    MutexLockGuard lock(*mutex_);
    if (file_.is_open())
    {
      file_.write(logline, len);
      file_.flush(); // 确保内容立即写入文件
      writtenBytes_ += len;
      if (writtenBytes_ > rollSize_)
        rollFile(false);
    }
  }
  void LogFile::flush()
  {
    if (file_.is_open())
    {
      file_.flush();
    }
  }

} // namespace zfwmuduo
