#include "../Poller.h"
#include "EPollPoller.h"
#include <stdlib.h> // getenv()
/**
 * 属于poller的公共源文件
 * 但是本身的Poller抽象层类的定义：派生类可以去使用它, 但是它不要去使用派生类
 */
namespace zfwmuduo
{
  Poller *Poller::newDefaultPoller(EventLoop *loop)
  {
    if (::getenv("MUDUO_USE_POLL"))
      return nullptr; // 生成poll实例
    else
      return new EPollPoller(loop); // 生成epoll实例
  }
} // namespace zfwmuduo