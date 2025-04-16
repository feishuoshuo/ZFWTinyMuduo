#pragma once

#include "../../base/Mutex.h"
#include "../../base/Condition.h"
#include "../../base/noncopyable.h"
#include "../../base/Thread.h"

#include <cstdlib> // atexit()
/**
 * 单例模式[线性安全的懒汉模式 initialization]
 * 在该实现中，没有考虑对象的销毁
 */

namespace zfwmuduo
{
  template <typename T>
  class Singleton : zfwmuduo::noncopyable
  {
  public:
    static T &instance()
    {
      // NOTE: 用于确保某个操作只执行一次。它常用于实现线程安全的单例模式初始化
      pthread_once(&ponce_, &Singleton::init);
      return *value_;
    }

  private:
    Singleton(); // 私有构造函数
    ~Singleton();

    static void init()
    {
      value_ = new T();
      // NOTE: atexit 允许在程序退出时（无论是正常退出还是通过 exit 函数退出）自动调用指定的函数，从而完成一些清理工作
      //  例如释放资源、关闭文件、销毁单例对象等
      ::atexit(destroy);
    }
    static void destroy()
    {
      delete value_;
    }

    // 确保某个操作在多线程环境中只执行一次
    // TAG: pthread_once_t来保证lazy-initialization的线程安全
    static pthread_once_t ponce_;
    static T *value_;
  };

  // NOTE: PTHREAD_ONCE_INIT标记“未初始化”状态
  template <typename T>
  pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT;

  template <typename T>
  T *Singleton<T>::value_ = nullptr;

} // namespace zfwmuduo