#include <functional> // bind()
#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

namespace zfwmuduo
{
  // 不接受用户传一个空指针给loop_
  static EventLoop *CheckLoopNotNull(EventLoop *loop)
  {
    if (!loop)
    {
      LOG_FATAL("%s:%s:%d mainloop is null! \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
  }

  TcpConnection::TcpConnection(EventLoop *loop,
                               const std::string &name,
                               int sockfd,
                               const InetAddress &localAddr,
                               const InetAddress &peerAddr) : loop_(CheckLoopNotNull(loop)),
                                                              name_(name),
                                                              state_(kConnecting),
                                                              reading_(true),
                                                              socket_(new Socket(sockfd)),
                                                              channel_(new Channel(loop, sockfd)),
                                                              localAddr_(localAddr),
                                                              peerAddr_(peerAddr),
                                                              highWaterMark_(64 * 1024 * 1024)
  {
    // 下面给channel设置相应的回调函数, poller给channel通知感兴趣的事件发生了, channel会回调相应的操作函数
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));

    LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", name_.c_str(), sockfd);
    socket_->setKeepAlive(true); // 启动tcp socket的保活机制
  }
  TcpConnection::~TcpConnection()
  {
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d \n", name_.c_str(), channel_->fd(), state_);
  }

  void TcpConnection::send(const void *message, int len)
  {
  }

  void TcpConnection::shutdown()
  {
  }
} // namespace zfwmuduo