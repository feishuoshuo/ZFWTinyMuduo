#include "EPollPoller.h"
#include "../../base/Logger.h"
#include "../Channel.h"
#include "errno.h"   // errno
#include <strings.h> // bzero()
#include <unistd.h>  //close

namespace zfwmuduo
{
  // 状态信息(channel的index_为下面的状态标记)
  // channel未添加到poller中
  const int kNew = -1;
  // channel已添加到poller中
  const int kAdded = 1;
  // channel从poller中删除
  const int kDeleted = 2;

  EPollPoller::EPollPoller(EventLoop *loop) : Poller(loop),
                                              epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
                                              events_(kInitEventListSize) // vector<epoll_event>
  {
    if (epollfd_ < 0)
    {
      LOG_FATAL("epoll_create error: %d \n", errno);
    }
  }
  EPollPoller::~EPollPoller()
  {
    ::close(epollfd_);
  }

  // channel update remove => EventLoop updateChannel removeChannel => Poller updateChannel removeChannel
  /**
   *                             EventLoop
   *                            /          \
   *   ChannelList(包含所有Channel)          Poller
   *                               ChannelMap <fd,channel*> (注册过的Channel)
   */
  void EPollPoller::updateChannel(Channel *channel)
  {
    const int index = channel->index();
    LOG_INFO("func=%s => fd=%d events=%d index=%d \n", __FUNCTION__, channel->fd(), channel->events(), index);
    if (index == kNew || index == kDeleted)
    {
      if (index == kNew)
      {
        int fd = channel->fd();
        channels_[fd] = channel;
      }

      channel->set_index(kAdded);
      update(EPOLL_CTL_ADD, channel);
    }
    else
    { // channel已在poller上注册过
      int fd = channel->fd();
      if (channel->isNoneEvent())
      {
        update(EPOLL_CTL_DEL, channel);
        channel->set_index(kDeleted);
      }
      else
        update(EPOLL_CTL_MOD, channel);
    }
  }

  // 从poller中移除channel
  void EPollPoller::removeChannel(Channel *channel)
  {
    int fd = channel->fd();
    channels_.erase(fd);

    LOG_INFO("func=%s => fd=%d \n", __FUNCTION__, fd);

    int index = channel->index();
    if (index == kAdded)
      update(EPOLL_CTL_DEL, channel);
    channel->set_index(kNew);
  }

  void EPollPoller::update(int operation, Channel *channel)
  {
    epoll_event event;
    bzero(&event, sizeof event); // 清零
    int fd = channel->fd();

    event.events = channel->events();
    event.data.fd = fd;
    event.data.ptr = channel;

    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
      if (operation == EPOLL_CTL_DEL)
      {
        LOG_ERROR("epoll_ctl del error:%d\n", errno);
      }
      else
      {
        LOG_FATAL("epoll_ctl del error:%d\n", errno);
      }
    }
  }

  Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
  {
    // TODO: 以下LOG_INFO, 实际应用LOG_DEBUG输出日志更为合理 否则频繁LOG_INFO会造成调用效率下降
    LOG_INFO("func=%s => fd total count:%lu \n", __FUNCTION__, channels_.size());

    // 第二个参数要求是：epoll_event *__events发生事件的fd的events数组的首地址
    // NOTE: &*events_.begin() 调用vector底层首元素的起始地址
    int numEvents = ::epoll_wait(epollfd_,
                                 &*events_.begin(),
                                 static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());
    if (numEvents > 0)
    { // 有发生事件
      LOG_INFO("%d events happened \n", numEvents);
      fillActiveChannels(numEvents, activeChannels);
      if (numEvents == events_.size()) // vector当前容量不够，需要扩容
      {
        events_.resize(events_.size() * 2); // 加倍扩容
      }
    }
    else if (numEvents == 0)
    { // 超时
      LOG_DEBUG("%s timeout! \n", __FUNCTION__);
    }
    else
    {
      if (saveErrno != EINTR)
      {
        errno = saveErrno; // errno重置成当前loop发生错误的值。因为errno是全局，它现在的值不一定是当前发生的错误，所以要适配的更新一下
        LOG_ERROR("EPollPoller::poll()");
      }
    }
    return now;
  }

  void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels)
  {
    for (int i = 0; i < numEvents; ++i)
    {
      Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
      channel->set_revents(events_[i].events);
      activeChannels->push_back(channel); // EventLoop此时拿到它的poller给它让返回的所有发生事件的channel列表
    }
  }

} // namespace zfwmuduo