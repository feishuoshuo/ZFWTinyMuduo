#include "Thread.h"
#include "CurrentThread.h"
#include <semaphore.h> //信号量的处理 sem_t、sem_init()、sem_wait()、sem_post()

namespace zfwmuduo
{
  // NOTE：静态变量要在类外单独进行初始化！注意！std::atomic 类型不允许拷贝构造(=0不可取！)
  std::atomic_int Thread::numCreated_(0);

  Thread::Thread(ThreadFunc func, const std::string &name) : started_(false),
                                                             joined_(false),
                                                             tid_(0),
                                                             func_(std::move(func)),
                                                             name_(name) { setDefaultName(); }
  Thread::~Thread()
  {
    if (started_ && !joined_)
    {
      // 分离线程(thread类提供了设置分离线程的方法)
      thread_->detach(); // 相当于pthread_detach();
    }
  }

  // TAG：多线程编程：通过信号量（sem_t）同步线程的启动过程，确保主线程能够获取新线程的线程ID
  void Thread::start() // 一个Thread对象, 记录的就是一个新线程的详细信息
  {
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0); // false表示信号量仅在当前进程内部使用； 0表示当前没有信号量可以获取

    // 开启线程
    //  NOTE：lambda表达式: 以引用的方式接收外部的对象
    thread_ = std::shared_ptr<std::thread>(new std::thread([&]() { // 启动新线程, 获取线程的tid值
      tid_ = CurrentThread::tid();

      sem_post(&sem); // 信号量通知：释放一个信号量，通知主线程线程ID已经准备好

      // 专门执行该线程函数(用户传入的线程任务)
      func_();
    }));

    // 这里必须等待获取上面新创建的线程的tid值(这一步确保主线程在新线程启动并获取线程ID之后才继续执行)
    sem_wait(&sem); // 阻塞当前线程，直到信号量的值大于 0
  }

  void Thread::join()
  {
    joined_ = true;
    thread_->join(); // 相当于pthread_join();
  }

  void Thread::setDefaultName()
  {
    int num = ++numCreated_;
    if (name_.empty())
    { // 线程还未设置名字
      char buf[32] = {0};
      snprintf(buf, sizeof buf, "Thread%d", num);
    }
  }
} // namespace zfwmuduo