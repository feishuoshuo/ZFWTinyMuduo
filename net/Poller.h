#pragma once

#include "../base/noncopyable.h"
#include "../base/Timestamp.h"
#include <vector>
#include <unordered_map>
/**
 * muduo库中多路事件分发器Demultiplex的核心:IO多路复用模块
 * 即开启事件循环epoll_wait
 */

namespace zfwmuduo
{
  class Channel;
  class EventLoop;
  class Poller : noncopyable
  {
  public:
    typedef std::vector<Channel *> ChannelList;
    Poller(EventLoop *loop);
    // TAG: [编程好习惯] 基类的析构函数必须是虚函数，否则可能会导致内存资源释放的不完全
    virtual ~Poller() = default;

    // 给所有IO复用保留统一的接口
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;

    // 判断参数channel是否在当前Poller当中
    bool hasChannel(Channel *channel) const;

    // EventLoop可以通过该接口获取默认的IO复用的具体实现
    static Poller *newDefaultPoller(EventLoop *loop);

  protected:
    typedef std::unordered_map<int, Channel *> ChannelMap; // key: sockfd ; value:sockfd所属的channel类型
    ChannelMap channels_;

  private:
    EventLoop *ownerLoop_; // 定义Poller所属的事件循环EventLoop
  };

} // namespace zfwmuduo