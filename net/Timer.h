#pragma once
#include <atomic> // AtomicInt64
#include "../base/noncopyable.h"
#include "../base/Timestamp.h"
#include "Callbacks.h" // TimerCallback

/**
 * fetch_add是一个原子操作，它会将指定的值（这里是1）加到当前值上，并返回加之前的值
 * std::memory_order_relaxed 是一个内存序，表示这个操作不需要任何内存顺序保证。在计数器的场景中，这通常是足够的。
 */
namespace zfwmuduo
{
  class Timer : zfwmuduo::noncopyable
  {
  public:
    Timer(zfwmuduo::TimerCallback cb, zfwmuduo::Timestamp when,
          double interval)
        : callback_(std::move(cb)),
          expiration_(when),
          interval_(interval),
          repeat_(interval > 0.0),
          sequence_(s_numCrteated_.fetch_add(1, std::memory_order_relaxed) + 1) {}

    void run() const { callback_(); }

    zfwmuduo::Timestamp expiration() const { return expiration_; }
    bool repeat() const { return repeat_; }
    int64_t sequence() const { return sequence_; }

    void restart(zfwmuduo::Timestamp now);

    static int64_t numCreated() { return s_numCrteated_.load(std::memory_order_relaxed); }

  private:
    const zfwmuduo::TimerCallback callback_;
    zfwmuduo::Timestamp expiration_;
    const double interval_;
    const bool repeat_;
    const int64_t sequence_; // 全局递增序列

    static std::atomic<int64_t> s_numCrteated_; // 用于记录创建的Timer对象的总数
  };

} // namespace zfwmuduo