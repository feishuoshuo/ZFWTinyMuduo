#pragma once

#include <string>
#include "noncopyable.h"

/**
 * Logger日志实现
 */
namespace zfwmuduo
{
  // 输出一个日志类
  class Logger : noncopyable
  {
  private:
    int logLevel_; // 日志级别
    Logger() {}

  public:
    // 定义日志的级别
    enum LogLevel
    {
      INFO,  // 普通信息
      ERROR, // 错误信息
      FATAL, // core信息
      DEBUG, // 调试信息
    };

    // 获取日志唯一的实例对象
    static Logger &instance();
    // 设置日志级别
    void setLogLevel(int level);
    // 写日志 [级别信息] time : msg
    void log(std::string msg);
  };

  // Log_INFO("%s %d", arg1, arg2)
#define LOG_INFO(logmsgFormat, ...)                   \
  do                                                  \
  {                                                   \
    Logger &logger = Logger::instance();              \
    Logger.setLogLevel(INFO);                         \
    char buf[1024] = {0};                             \
    snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
    logger.log(buf);                                  \
  } while (0);

#define LOG_ERROR(logmsgFormat, ...)                  \
  do                                                  \
  {                                                   \
    Logger &logger = Logger::instance();              \
    Logger.setLogLevel(ERROR);                        \
    char buf[1024] = {0};                             \
    snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
    logger.log(buf);                                  \
  } while (0);

#define LOG_FATAL(logmsgFormat, ...)                  \
  do                                                  \
  {                                                   \
    Logger &logger = Logger::instance();              \
    Logger.setLogLevel(FATAL);                        \
    char buf[1024] = {0};                             \
    snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
    logger.log(buf);                                  \
  } while (0);

// NOTE：由于调试频率很高，大量宏定义会造成软件运行负担，因此使用 #ifdef
#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat, ...)                  \
  do                                                  \
  {                                                   \
    Logger &logger = Logger::instance();              \
    Logger.setLogLevel(DEBUG);                        \
    char buf[1024] = {0};                             \
    snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
    logger.log(buf);                                  \
  } while (0);
#else
#define LOG_DEBUG(logmsgFormat, ...)
#endif

} // namespace zfwmuduo