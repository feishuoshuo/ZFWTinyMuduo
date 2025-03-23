#include "CurrentThread.h"
#include <unistd.h> // syscall()
#include <sys/syscall.h>

namespace zfwmuduo
{
  namespace currentThread
  {
    // 变量初初始化
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