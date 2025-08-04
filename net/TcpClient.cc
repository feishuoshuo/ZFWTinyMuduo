#include "TcpClient.h"
#include "Connector.h"
#include "TcpConnection.h"
#include "../base/Logger.h"
#include <functional> // bind

namespace zfwmuduo
{
  void removeConnector(const ConnectorPtr &connector)
  {
    connector->remove();
  }

  void removeConnection(EventLoop *loop, const TcpConnectionPtr &conn)
  {
    loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
  }

  TcpClient::TcpClient(EventLoop *loop,
                       const InetAddress &serverAddr,
                       const std::string nameArg)
      : loop_(loop),
        connector_(new Connector(loop, serverAddr))
  {
    connector_->setNewConnectionCallback(
        std::bind(&TcpClient::newConnction, this, std::placeholders::_1));
    LOG_INFO("TcpClient::TcpClient[ %s ] - connector ", this);
  }
  TcpClient::~TcpClient()
  {
    LOG_INFO("TcpClient::~TcpClient[ %s ] - connector ", this);
    // 1-获取当前连接
    TcpConnectionPtr conn;
    {
      MutexLockGuard lock(mutex_);
      conn = connection_;
    }

    if (conn) // 2-存在连接时的处理
    {
      // 确保在事件循环的线程中执行关闭回调
      auto cb = [loop = loop_](const TcpConnectionPtr &conn)
      { removeConnection(loop, conn); };

      // mutable 关键字允许 Lambda 表达式修改捕获的变量
      loop_->runInLoop([conn, cb]() mutable
                       { conn->setCloseCallback(cb); });
    }
    else
    { // 3-不存在连接时的处理
      connector_->stop();
      loop_->runAfter([connector = connector_]() mutable
                      { removeConnector(connector); });
    }
  }

  void TcpClient::connect();
  void TcpClient::disconnect();
  void TcpClient::stop();
} // namespace zfwmuduo