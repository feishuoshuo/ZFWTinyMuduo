#include "TimerQueue.h"
#include "../base/Logger.h" // LOG_ERROR, LOG_INFO
#include "EventLoop.h"
#include "Timer.h"
#include "TimerId.h"

#include <functional>    //bind
#include <sys/timerfd.h> // timerfd_create(), TFD_NONBLOCK, TFD_CLOEXEC, itimerspec
#include <unistd.h>      // 关闭文件描述符close()
#include <cstring>       // memZero()

namespace zfwmuduo
{
  // 计算从当前时间到指定时间点的时间差，并将结果转换为 timespec 结构体
  timespec howMuchTimeFromNow(Timestamp when)
  {
    int64_t microseconds = when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
    if (microseconds < 1000)
    {
      microseconds = 1000;
    }
    timespec ts; // tv_sec:秒数; tv_nsec:纳秒数
    ts.tv_sec = static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>((microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
    return ts;
  }

  int createTimerfd()
  {
    // NOTE: timerfd_create()创建一个定时器文件描述符，该描述符可以用来设置定时器，并在指定的时间间隔后生成一个事件
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
                                   TFD_NONBLOCK | TFD_CLOEXEC); // 非阻塞模式 | 在执行exec时自动关闭
    if (timerfd < 0)
    {
      LOG_ERROR("Failed in timerfd_create");
    }
    return timerfd;
  }

  void readTimerfd(int timerfd, Timestamp now)
  {
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
    LOG_INFO("TimerQueue::handleRead() reads %zd ytes instead of 8", n);
  }

  // 重置一个 timerfd 文件描述符的定时器
  void resetTimerfd(int timerfd, Timestamp expiration)
  {
    // 通过timerfd_settime()唤醒loop
    // NOTE: itimerspec 通常用于 timerfd_settime 系统调用，设置定时器的参数
    struct itimerspec newValue;
    struct itimerspec oldValue;
    // 清零内存区域
    std::memset(&newValue, 0, sizeof newValue);
    std::memset(&oldValue, 0, sizeof oldValue);

    newValue.it_value = howMuchTimeFromNow(expiration);
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if (ret)
    {
      LOG_ERROR("timerfd_settime()");
    }
  }

  //=======================================
  TimerQueue::TimerQueue(EventLoop *loop) : loop_(loop),
                                            timerfd_(createTimerfd()),
                                            timerfdChannel_(loop, timerfd_),
                                            timers_(),
                                            callingExpiredTimers_(false)
  {
    timerfdChannel_.setReadCallback(
        std::bind(&TimerQueue::handleRead, this));
    timerfdChannel_.enableReading();
  }
  TimerQueue::~TimerQueue()
  {
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);
    // 不要移除Channel, 因为我们正处于 EventLoop 的析构函数中
    for (const Entry &timer : timers_)
    {
      delete timer.second;
    }
  }

  TimerId TimerQueue::addTimer(const zfwmuduo::TimerCallback &cb,
                               zfwmuduo::Timestamp when,
                               double interval)
  {
    Timer *timer = new Timer(std::move(cb), when, interval);
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
    return TimerId(timer, timer->sequence());
  }

  void TimerQueue::cancel(TimerId timerId)
  {
    loop_->runInLoop(std::bind(&TimerQueue::cancelInLoop, this, timerId));
  }
  void TimerQueue::addTimerInLoop(Timer *timer)
  {
    bool earliestChanged = insert(timer);
    if (earliestChanged)
      resetTimerfd(timerfd_, timer->expiration());
  }
  void TimerQueue::cancelInLoop(TimerId timerId)
  {
    ActiveTimer timer(timerId.timer_, timerId.sequence_);
    ActiveTimerSet::iterator it = activeTimers_.find(timer);
    if (it != activeTimers_.end())
    {
      size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
      delete it->first;
      activeTimers_.erase(it);
    }
    else if (callingExpiredTimers_)
    {
      cancelingTimers_.insert(timer);
    }
  }
  void TimerQueue::handleRead()
  {
    Timestamp now(Timestamp::now());
    readTimerfd(timerfd_, now);

    std::vector<Entry> expired = getExpired(now);

    callingExpiredTimers_ = true;
    cancelingTimers_.clear();
    // 在临界区之外调用回调函数是安全的
    for (const Entry &it : expired)
    {
      it.second->run();
    }
    callingExpiredTimers_ = false;

    reset(expired, now);
  }
  std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
  {
    std::vector<Entry> expired;
    // 哨兵条目: 帮助定位队列中第一个未到期的定时器
    Entry sentry = std::make_pair(now, reinterpret_cast<Timer *>(UINTPTR_MAX));
    TimerList::iterator it = timers_.lower_bound(sentry); // lower_bound找第一个不小于的
    // NOTE:back_inserter 的作用是将元素插入到容器的末尾，而不是覆盖容器中的现有元素。它通过调用容器的 push_back 方法来实现这一点
    std::copy(timers_.begin(), it, back_inserter(expired));
    timers_.erase(timers_.begin(), it);

    return expired;
  }
  void TimerQueue::reset(const std::vector<Entry> &expired, Timestamp now)
  {
    Timestamp nextExpire;
    // 遍历已到期的定时器
    for (auto it = expired.begin(); it != expired.end(); ++it)
    {
      if (it->second->repeat())
      {
        it->second->restart(now);
        insert(it->second);
      }
      else
      {
        delete it->second;
      }

      if (!timers_.empty())
      {
        nextExpire = timers_.begin()->second->expiration();
      }
      if (nextExpire.valid())
      {
        resetTimerfd(timerfd_, nextExpire);
      }
    }
  }
  bool TimerQueue::insert(Timer *timer)
  {
    bool earliestChanged = false; // 标记是否需要更新最早到期的定时器
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    if (it == timers_.end() || when < it->first)
    { // 若定时器队列为空,或者队列中均为无效定时器-->需要更新最早到期的定时器
      earliestChanged = true;
    }
    std::pair<TimerList::iterator, bool> result =
        timers_.insert(std::make_pair(when, timer));

    return earliestChanged;
  }

} // namespace zfwmuduo