#include "Connector.h"
#include "EventLoop.h"
#include "../base/Logger.h"
#include <functional> // bind

namespace zfwmuduo
{
  static int createNonblocking()
  {
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sockfd < 0)
      LOG_FATAL("%s:%s:%d listen socket create errno:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
    return sockfd;
  }

  Connector::Connector(EventLoop *loop, const InetAddress &serverAddr)
      : loop_(loop),
        serverAddr_(serverAddr),
        connect_(false),
        state_(kDisconnected),
        retryDelayMs_(kInitRetryDelayMs)
  {
    LOG_DEBUG("ctor[ %s ] \n", this);
  }

  Connector::~Connector()
  {
    LOG_DEBUG("dtor[ %s ] \n", this);
    loop_->cancel(timerId_);
  }

  void Connector::start()
  {
    connect_ = true;
    loop_->runInLoop(std::bind(&Connector::startInLoop, this));
  }

  void Connector::restart()
  {
    setState(kDisconnected); // 重置连接状态为未连接
    retryDelayMs_ = kInitRetryDelayMs;
    connect_ = true;
    startInLoop();
  }

  void Connector::stop()
  {
    connect_ = false;
    loop_->cancel(timerId_);
  }

  void Connector::remove()
  {
    loop_->runInLoop([this]()
                     {
      stop();
      if (timerId_.valid())
      {
        loop_->cancel(timerId_); // 移除定时器
      } });
  }

  void Connector::startInLoop()
  {
    if (connect_)
      connect();
    else
      LOG_DEBUG("do not connect\n");
  }

  void Connector::connect()
  {
    // 将 sockaddr_in 转换为 sockaddr
    const struct sockaddr *sockaddr_ptr = reinterpret_cast<const struct sockaddr *>(serverAddr_.getSockAddr());

    int sockfd = createNonblocking();
    int res = ::connect(sockfd, sockaddr_ptr, sizeof serverAddr_);
    // 检查连接结果
    if (res == -1)
    {
      LOG_ERROR("connect error in Connector::startInLoop %d", errno);
    }
    else
    {
      LOG_INFO("Connection established.");
    }
  }

} // namespace zfwmuduo