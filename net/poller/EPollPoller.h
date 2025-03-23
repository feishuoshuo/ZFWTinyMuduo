#pragma once

#include "../Poller.h"
#include "../../base/Timestamp.h"
#include <vector>
#include <sys/epoll.h> // epoll_event
/**
 * epoll的使用：epoll_create、epoll_ctl[add/mod/del]、epoll_wait
 */

namespace zfwmuduo
{
  class EventLoop;
  class Channel;
  class EPollPoller : public Poller
  {
  public:
    EPollPoller(EventLoop *loop); // 1- epoll_create
    ~EPollPoller() override;

    // 重写基类poller的抽象方法
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;
    // 3- epoll_wait
    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;

  private:
    static const int kInitEventListSize = 16;

    // 填写活跃的连接
    void fillActiveChannels(int numEvents, ChannelList *activeChannels);
    // 更新channel通道 2- epoll_ctl add/mod//del
    void update(int operation, Channel *channel);

    typedef std::vector<epoll_event> EventList;

    int epollfd_; // 指定要访问的内核事件表
    EventList events_;
  };

} // namespace zfwmuduo