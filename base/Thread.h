#pragma once

#include "noncopyable.h"
#include <functional> // function
#include <thread>     // C++11提供的线程类
#include <memory>     // shared_ptr
#include <unistd.h>   // pid_t
#include <atomic>     // atomic_int
#include <string>
/**
 * Thread 仅关注一个线程
 */

namespace zfwmuduo
{
  class Thread : noncopyable
  {
  public:
    typedef std::function<void()> ThreadFunc; // 线程函数

    explicit Thread(ThreadFunc, const std::string &name = std::string());
    ~Thread();

    void start(); // 调用它才开始创建子线程
    void join();

    bool started() const { return started_; }
    pid_t pid() const { return tid_; }
    const std::string &name() const { return name_; }

    static int numCreated() { return numCreated_; }

  private:
    void setDefaultName(); // 给线程设置默认名称

    bool started_;
    bool joined_; // 表示线程是否已经被“join”（即等待线程结束）

    // 这里使用智能指针封装它,以便控制线程的启动
    // TAG：因为如果std::thread thread_, 则创建之后就直接启动了
    std::shared_ptr<std::thread> thread_;
    pid_t tid_;
    ThreadFunc func_; // 存储线程函数
    std::string name_;
    static std::atomic_int numCreated_;
  };

} // namespace zfwmuduo