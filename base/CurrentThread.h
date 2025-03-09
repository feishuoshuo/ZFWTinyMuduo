#pragma once

#include <unistd.h>
#include <sys/syscall.h>
/**
 * 线程
 */

namespace zfwmuduo
{

  namespace CurrentThread
  {
    // internal
    // NOTE: __thread相当于thread_local, 表示变量 t_cachedTid 是线程局部的，每个线程都有自己的独立副本
    // NOTE: extern 主要用来告诉编译器，某个变量或函数的定义在其他地方（可能是其他文件或库中），而不是当前文件中。
    extern __thread int t_cachedTid;
    extern __thread char t_tidString[32];
    extern __thread int t_tidStringLength;
    extern __thread const char *t_threadName;
    void cacheTid();

    inline int tid()
    {
      if (__builtin_expect(t_cachedTid == 0, 0))
      {
        cacheTid();
      }
      return t_cachedTid;
    }

  } // namespace CurrentThread
} // namespace zfwmuduo