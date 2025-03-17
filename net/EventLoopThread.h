#pragma once

#include <functional> // function
#include <string>
#include <mutex>              // 互斥锁
#include <condition_variable> // 条件变量
#include "Thread.h"
#include "../base/noncopyable.h"
/**
 * EventLoopThread: 事件线程类
 * 一个线程一个循环
 */

namespace zfwmuduo
{
  class EventLoop;
  class EventLoopThread : noncopyable
  {
  public:
    typedef std::function<void(EventLoop *)> ThreadInitCallback;

    // ThreadInitCallback()就表示创建一个默认的 std::function 对象。std::function 的默认构造函数会创建一个空的函数对象，表示没有绑定任何函数或可调用对象。
    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),
                    const std::string &name = std::string());
    ~EventLoopThread();
    EventLoop *startLoop();

  private:
    void threadFunc();

    EventLoop *loop_;
    bool exiting_; // 标识是否退出循环
    zfwmuduo::Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;
  };

} // namespace zfwmuduo