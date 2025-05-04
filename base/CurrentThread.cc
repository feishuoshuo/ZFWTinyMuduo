#include "CurrentThread.h"
#include <unistd.h> // syscall()
#include <sys/syscall.h>

namespace zfwmuduo
{
  /**
   * 用__thread变量来缓存gettid()的返回值，这样只有在本线程第一次调用时才进行系统调用，
   * 以后都是直接从thread local缓存的线程id拿到结果
   */
  namespace currentThread
  {
    // NOTE：变量初初始化 __thread为每个线程创建独立的变量副本
    __thread int t_cachedTid = 0;
    __thread char t_tidString[32] = {0};
    __thread int t_tidStringLength = 0;
    __thread const char *t_threadName = "unknown";

    void cacheTid()
    {
      if (t_cachedTid == 0)
      { // 通过linux系统调用, 获取当前线程的tid值
        t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
      }
    }
  }

} // namespace zfwmuduo