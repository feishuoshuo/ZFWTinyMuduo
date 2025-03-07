/**
 * muduo网络库给用户提供了两个主要的类
 * TcpServer：用于编写服务器程序
 * TcpClient：用于编写客户端程序
 *
 * epoll + 线程池
 * 好处：能够把网路IO的代码和业务代码区分开
 *                         用户连接和断开onConnection、用户可读写事件onMessage【主要的编写精力在这里】
 */
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/base/Logging.h>
#include <iostream>
#include <string>
#include <functional> //bind绑定器
using namespace std;
using namespace placeholders; //_1参数占位符

/**
 * 基于muduo网络库开发服务器程序
 * 1.组合TcpServer对象
 * 2.创建EventLoop事件循环对象的指针
 * 3.明确TcpServer构造函数需要什么参数，输出ChatServer的构造函数
 * 4.在当前服务器类ChatServer的构造函数中，注册处理连接的回调函数和处理读写时间的回调函数
 * 5.设置何是的服务端线程数量，muduo库会自己划分IO线程和worker线程
 */
class ChatServer
{
private:
  muduo::net::TcpServer _server; // #1 [注意TcpServer没有默认构造！]
  muduo::net::EventLoop *_loop;  // #2 epoll

  // 4.1专门处理用户连接创建和断开 epoll listenfd accept
  void onConnection(const muduo::net::TcpConnectionPtr &conn)
  {
    if (conn->connected())
    {
      cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << "state:online" << endl;
    }
    else
    {
      cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << "state:offline" << endl;
      conn->shutdown(); // 连接断开将socket资源释放   相当于socket中的·close(fd)
      // 或者调用_loop->quit()退出epoll;
    }
  }

  // 4.2 专门处理用户读写事件
  void onMessage(const muduo::net::TcpConnectionPtr &conn, // 连接
                 muduo::net::Buffer *buffer,               // 缓冲区
                 muduo::Timestamp time)                    // 接收到数据的时间信息
  {
    string buf = buffer->retrieveAllAsString(); // 将接收数据全部放入字符串中
    cout << "recv data:" << buf << " time:" << time.toString() << endl;
    conn->send(buf); // 收到什么数据发回去什么数据
  }

public:
  ChatServer(muduo::net::EventLoop *loop,               // 事件循环
             const muduo::net::InetAddress &listenAddr, // 绑定IP地址 + 端口号
             const string &nameArg                      // TcpServer服务器名字
             ) : _server(loop, listenAddr, nameArg), _loop(loop)
  {
    // 4.1给服务器注册用户连接的创建和断开事件的回调
    // NOTE: “回调”是一种编程技术，指的是将一个函数作为参数传递给另一个函数，并在适当的时机被调用。回调函数通常用于处理异步操作、事件驱动编程或函数之间的交互
    // NOTE: 利用绑定器绑定成员方法onConnection,保持参数与muduo库函数参数一致
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

    // 4.2给服务器注册用户读写事件的回调
    // 利用绑定器绑定成员方法onMessage,保持参数与muduo库函数参数一致
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 5.设置服务器端的线程数量 1个IO线程 3个worker线程
    _server.setThreadNum(4);
  }

  // 6.开启事件循环
  void start()
  {
    _server.start();
  }
};

int main()
{
  muduo::net::EventLoop loop; // epoll
  muduo::net::InetAddress addr("127.0.0.1", 6000);
  ChatServer server(&loop, addr, "ChatServer");

  server.start(); // 启动服务：listenfd通过epoll_ctl添加到epoll上
  loop.loop();    // 类似于epoll_wait以阻塞的方式等待新用户连接或处理已连接用户的读写事件

  return 0;
}