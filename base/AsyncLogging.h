#pragma once

#include <string>
#include <vector>
#include <memory> //unique_ptr
#include "noncopyable.h"
#include "Thread.h"
#include "CountDownLatch.h"
#include "Condition.h"
#include "Mutex.h"
/**
 * 多线程异步日志/非阻塞日志
 *
 * 多线程程序对日志库的新需求：线程安全，
 * 即多个线程可并发写日志，两个线程不会出现交织
 * ==========================================
 * “异步日志”:
 * 一个背景线程负责收集日志消息，并写入日志文件
 * 其他业务线程只负责该“日志线程”发送日志消息
 *
 * 双缓冲double buffering技术
 * 基本思想：两个buffer(A and B)
 * 前端-往A填数据(日志消息)
 * 后端-将B数据写入文件
 * A满后，交换A B
 */
namespace zfwmuduo
{
  class Buffer; // TODO

  class AsyncLogging : zfwmuduo::noncopyable
  {
  public:
    AsyncLogging(const std::string &basename, off_t rollSize, int flushInterval = 3);
    ~AsyncLogging()
    {
      if (running_)
        stop();
    }

    void append(const char *logline, int len);

    void start()
    {
      running_ = true;
    }
    void stop() // NO_THREAD_SAFETY_ANALYSIS
    {
      running_ = false;
      cond_.notify(); // 通知等待的线程
      if (thread_.joinable())
        thread_.join(); // 确保了调用 stop 的线程会阻塞，直到被停止的线程完全退出
    }

  private:
    // 用于实现日志系统中的异步日志写入功能
    void threadFunc();

    typedef std::vector<std::unique_ptr<Buffer>> BufferVector;
    typedef BufferVector::value_type BufferPtr; // 萃取出元素类型

    zfwmuduo::MutexLock mutex_;
    zfwmuduo::Condition cond_;
    BufferPtr currentBuffer_; // 当前缓冲区A
    BufferPtr nextBuffer_;    // 预备缓冲区B
    BufferVector buffers_;    // 待写入文件的已填满的缓冲

    bool running_;
    std::string basename_;    // 日志文件的基本名称
    size_t rollSize_;         // 日志文件的最大大小，当达到这个大小时，日志文件会滚动
    const int flushInterval_; // 刷新时间间隔
    zfwmuduo::Thread thread_;
    // zfwmuduo::CountDownLatch latch_;
  };

} // namespace zfwmuduo