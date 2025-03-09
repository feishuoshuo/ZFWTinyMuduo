#include "CurrentThread.h"
#include <unistd.h> // syscall()
#include <sys/syscall.h>

namespace zfwmuduo
{
  namespace CurrentThread
  {
    void cacheTid()
    {
      if (t_cachedTid == 0)
      { // 通过linux系统调用, 获取当前线程的tid值
        t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
      }
    }
  }

} // namespace zfwmuduo