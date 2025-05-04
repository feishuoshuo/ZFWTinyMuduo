#pragma once
#include <set>
#include <vector>
#include "../base/noncopyable.h"
#include "../base/Timestamp.h"
#include "Channel.h"
#include "Callbacks.h" // TimerCallback

/**
 * TimerQueue:定时器(timer)队列
 * 在前面Reactor基础上, 给EventLoop加上定时器功能。
 * 传统的Reactor通过控制select()和poll()的等待时间来实现定时,而现在Linux中的timerfd,
 * 我们可以利用和处理IO时间相同方式, 来处理定时, 代码一致性更佳
 *
 * 注意！TimerQueue的成员函数只能在其所属的IO线程调用,因此不必加锁
 */

namespace zfwmuduo
{
  class Timer;
  class TimerId;
  class EventLoop;

  class TimerQueue : zfwmuduo::noncopyable
  {
  public:
    explicit TimerQueue(EventLoop *loop);
    ~TimerQueue();

    // 在给定的时间点安排回调函数的执行,如果interval大于0.0,则会重复执行
    // 必须是线程安全.通常被其他线程所调用
    TimerId addTimer(const zfwmuduo::TimerCallback &cb,
                     zfwmuduo::Timestamp when,
                     double interval);

  private:
    // 这样即便两个Timer的到期时间享同,他们地址也必定不同
    typedef std::pair<Timestamp, Timer *> Entry;
    typedef std::set<Entry> TimerList;
    typedef std::pair<Timer *, int64_t> ActiveTimer;
    typedef std::set<ActiveTimer> ActiveTimerSet;

    void cancel(TimerId timerId);
    void addTimerInLoop(Timer *timer);
    void cancelInLoop(TimerId timerId);
    void handleRead(); // 某个函数或回调会在timerfd触发时被调用
    // 获取已到期的定时器条目, 并将其从队列中移除
    std::vector<Entry> getExpired(Timestamp now);
    // 处理一组已到期的定时器条目, 并根据需要重新插入重复定时器
    void reset(const std::vector<Entry> &expired, Timestamp now);
    // 插入定时器timer, 并判断是否需要更新最早到期的timer
    bool insert(Timer *timer);

    EventLoop *loop_;
    const int timerfd_;
    Channel timerfdChannel_; // 观察timerfd_上的readable事件
    TimerList timers_;       // 按到期时间排序的定时器列表

    // for cancel()
    ActiveTimerSet activeTimers_; // 按对象地址排序, 保存目前有效的Timer的指针
    ActiveTimerSet cancelingTimers_;
    bool callingExpiredTimers_;
  };

} // namespace zfwmuduo