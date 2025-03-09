#pragma once

#include <functional> // function
#include <vector>
#include <atomic> // atomic_bool
#include <memory> // unique_ptr
#include <mutex>  // mutex
#include "../base/noncopyable.h"
#include "../base/Timestamp.h"
#include "../base/CurrentThread.h"

/**
 * EventLoop：事件循环  <-- Reactor模型上对应Demultiplex(多路事件分发器)
 *
 * EventLoop包含 ChannelList(多个通道) + Poller
 * 核心组件：poller(epoll的抽象)、Channel通道[epoll所监听的fd + epoll所感兴趣的事件 + epoll_wait通知的事件]
 * 事件分发器Demultiplex模块的实现
 */

namespace zfwmuduo
{
  class Channel;
  class Poller;
  class EventLoop : noncopyable
  {
  public:
    typedef std::function<void()> Functor; // 回调函数

    EventLoop();
    ~EventLoop();

    // 开启/退出事件循环
    void loop();
    void quit();

    Timestamp pollReturnTime() const { return pollReturnTime_; }

    void runInLoop(Functor cb);   // 在当前loop中执行
    void queueInLoop(Functor cb); // 把cb放入队列中, 唤醒loop所在的线程, 执行cb

    void wakeup(); // 唤醒loop所在线程

    // EventLoop的方法 ==> Poller的方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // 判断EventLoop对象是否在自己线程中
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

  private:
    void handleRead();        // wakeup()
    void doPendingFunctors(); // 执行回调

    typedef std::vector<Channel *> ChannelList;

    // NOTE: atomic_bool原子操作的bool, 通过CAS实现的
    /* atomic */
    std::atomic_bool looping_;
    std::atomic_bool quit_; // 标识退出loop循环

    const pid_t threadId_; // 记录当前loop所在线程的ID

    Timestamp pollReturnTime_; // poller返回发生事件的channels的时间点
    std::unique_ptr<Poller> poller_;

    // 当mainloop获取一个新用户的channel, 通过轮询算法选择一个subloop, 通过该成员唤醒subloop处理channel
    // NOTE： wakeupFd_和线程安全队列的设计可以参考《Linux高性能服务器编程(游双)》-半同步/半反应堆模式！！！
    int wakeupFd_; // TAG: 这个设计非常巧妙！！另一种mainloop与subloop的通知方式是采用生产者-消费者的线程安全队列[简单、但是不高效]
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activateChannels_;
    // Channel *currentActiveChannel_;

    std::atomic_bool callingPendingFunctors_; // 标识当前loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_;    // 存储loop需要执行的所有的回调操作
    std::mutex mutex_;                        // 互斥锁, 用来保护上面vector容器的线程安全操作
  };

} // namespace zfwmuduo