#pragma once

#include <memory>     // shared_ptr
#include <functional> // function
#include "../base/Timestamp.h"
/**
 * 互斥锁
 */

namespace zfwmuduo
{
  class Buffer;
  class TcpConnection;

  typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

  typedef std::function<void(const TcpConnectionPtr &)> ConnectionCallback;
  typedef std::function<void(const TcpConnectionPtr &)> MessageCallback;
  typedef std::function<void(const TcpConnectionPtr &)> WriteCompleteCallback;

  typedef std::function<void(const TcpConnectionPtr &,
                             Buffer *,
                             Timestamp)>
      MessageCallback;
  typedef std::function<void(const TcpConnectionPtr &, size_t)> HighWaterMarkCallback;
  typedef std::function<void(const TcpConnectionPtr &, size_t)> CloseCallback;
} // namespace zfwmuduo