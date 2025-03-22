#include <functional> // bind()
#include <errno.h>    // errno
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>
#include <memory> // shared_from_this()
#include <string>

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
    // 事件循环通过 Poller（如 epoll）检测套接字的状态变化，并在适当的时机调用这些回调函数
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
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d \n", name_.c_str(), channel_->fd(), (int)state_);
  }

  void TcpConnection::handleRead(Timestamp receiveTime)
  {
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0)
    {
      // 已建立连接的用户, 有可读事件发生了, 调用用户传入的回调操作onMessage
      messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0)
    {
      handleClose();
    }
    else
    {
      errno = savedErrno;
      LOG_ERROR("TcpConnection::handleRead");
      handleError();
    }
  }

  void TcpConnection::handleWrite()
  {
    if (channel_->isWriting())
    {
      int savedErrno = 0;
      ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno);
      if (n > 0)
      {
        outputBuffer_.retrieve(n);
        if (outputBuffer_.readableBytes() == 0) // 表示发送完成
        {
          channel_->disableWriting();
          if (writeCompleteCallback_)
          { // 唤醒loop_对应的thread线程, 执行回调
            loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
          }
          if (state_ == kDisconnecting)
          { // 正在关闭状态
            shutdownInLoop();
          }
        }
        else
        {
          LOG_ERROR("TcpConnection::handleWrite");
        }
      }
    }
    else
    {
      LOG_ERROR("TcpConnection fd=%d is down, no more writing \n", channel_->fd());
    }
  }

  // poller => channel::closeCallback => TcpConnection::handleColse
  void TcpConnection::handleClose()
  {
    LOG_INFO("TcpConnection::handleClose fd=%d state=%d \n", channel_->fd(), (int)state_);
    setState(kDisconnected);
    channel_->disableAll();

    // NOTE: std::shared_from_this()：这是 std::enable_shared_from_this 类的成员函数，用于生成一个指向当前对象的 std::shared_ptr
    TcpConnectionPtr connPtr(shared_from_this()); // 注意! 这里不是创建对象, 而是通过智能指针指向当前对象!
    connectionCallback_(connPtr);                 // 执行连接关闭的回调
    closeCallback_(connPtr);                      // 关闭连接的回调 执行的是TcpServer::removeConnection回调方法
  }

  void TcpConnection::handleError()
  {
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    // 调用 getsockopt 函数来获取套接字（socket）的错误状态, 并将错误信息记录到日志中
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
      err = errno;
    }
    else
    {
      err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d \n", name_.c_str(), err);
  }

  void TcpConnection::send(const std::string &buf)
  {
    if (state_ == kConnected)
    { // 发送数据必须是已建立连接的状态
      if (loop_->isInLoopThread())
      {
        sendInLoop(buf.c_str(), buf.size());
      }
      else
      {
        loop_->runInLoop(std::bind(
            &TcpConnection::sendInLoop,
            this,
            buf.c_str(),
            buf.size()));
      }
    }
  }

  // 发送数据 应用写的快, 而内核发送数据慢, 因此需将发送数据写入缓冲区,且设置了水位回调
  void TcpConnection::sendInLoop(const void *data, size_t len)
  {
    ssize_t nwrote = 0;
    size_t remaining = len;  // 表示没发送完的数据
    bool faultError = false; // 记录了是否产生错误

    // 之前调用过该TcpConnection的shutdown, 不能再进行发送了
    if (state_ == kDisconnected)
    {
      LOG_ERROR("disconnected, give up writing!");
      return;
    }

    // 表示channel_第一次开始写数据, 而且缓冲区没有待发送数据
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
      nwrote = ::write(channel_->fd(), data, len);
      if (nwrote >= 0)
      {
        remaining = len - nwrote;
        if (remaining == 0 && writeCompleteCallback_)
        {
          // 既然在这里数据全部发送完成, 就不用再给channel设置epoll事件了
          loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
        }
      }
      else // nwrote < 0 也就是出错
      {
        nwrote = 0;
        // NOTE: EWOULDBLOCK: Operation would block, 表示当前的 I/O 操作无法立即完成，需要等待资源可用
        if (errno != EWOULDBLOCK)
        {
          LOG_ERROR("TcpConnection::sendInLoop");
          if (errno == EPIPE || errno == ECONNRESET)
          { // 对端有错误发生( SIGPIPE RESET)
            faultError = true;
          }
        }
      }
    }

    // 说明当前这一次write, 并没有把数据全部发送出去, 剩余数据需要保存到缓冲区中,
    // 然后给channel注册epoll事件, poller发现tcp的发送数据缓冲区有空间,
    // 会通知相应sock-channel, 调用wtiteCallback_回调方法
    // 即, 调用TcpConnection::handleWrite方法, 把发送缓冲区中的数据全部发送完成
    if (!faultError && remaining > 0)
    {
      // 目前发送缓冲区待发送的剩余数据长度
      size_t oldLen = outputBuffer_.readableBytes();
      if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_)
      {
        loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
      }
      outputBuffer_.append(static_cast<const char *>(data) + nwrote, remaining);
      if (!channel_->isWriting())
      {
        channel_->enableReading(); // 这里一定要注册channel的写事件, 否则poller不会给channel通知epollout
      }
    }
  }

  void TcpConnection::connectEstablished()
  {
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading(); // 向poller注册channel的epollin事件

    // 新连接建立, 执行回调
    connectionCallback_(shared_from_this());
  }

  void TcpConnection::connectDestroyed()
  {
    if (state_ == kConnected)
    {
      setState(kDisconnected);
      channel_->disableAll();                  // 将channel的所有感兴趣事件, 从poller中delete掉
      connectionCallback_(shared_from_this()); // 断开连接
    }
    channel_->remove(); // 把channel从poller中删除
  }

  void TcpConnection::shutdown()
  {
    if (state_ == kConnected)
    {
      setState(kDisconnecting);
      loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
  }

  void TcpConnection::shutdownInLoop()
  {
    if (!channel_->isWriting()) // 说明当前outputBuffer中的数据已经全部发送完成
    {
      socket_->shutdownWrite(); // 关闭写端
    }
  }

} // namespace zfwmuduo