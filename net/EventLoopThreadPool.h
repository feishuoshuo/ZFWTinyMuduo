#pragma once

#include <functional> // function
#include <string>
#include <vector>
#include <memory> // unique_ptr
#include "../base/noncopyable.h"

/**
 * EventLoopThreadPool: 线程池
 */

namespace zfwmuduo
{
  class EventLoop;
  class EventLoopThread;
  class EventLoopThreadPool : noncopyable
  {
  public:
    typedef std::function<void(EventLoop *)> ThreadInitCallback;

    EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();

    // 设置底层线程数量
    void setThreadNum(int numThreads) { numThreads_ = numThreads; }

    void start(const ThreadInitCallback &cb = ThreadInitCallback());

    // 若在多线程中, baseloop_默认以轮询的方式分配channel给subloop
    EventLoop *getNextLoop();

    // 提供了一个接口返回池子里的所有loops
    std::vector<EventLoop *> getAllLoops();

    bool started() const { return started_; }
    const std::string &name() const { return name_; }

  private:
    EventLoop *baseLoop_; // EventLoop loop
    std::string name_;
    bool started_;
    int numThreads_;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_; // 包含了所有创建的事件线程
    std::vector<EventLoop *> loops_;                        // 包含了事件线程中的EventLoop的指针
  };
} // namespace zfwmuduo