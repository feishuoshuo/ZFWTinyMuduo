#pragma once

/**
 * EventLoop：事件循环
 * 核心组件：poller(epoll的抽象)、Channel通道[epoll所监听的fd + epoll所感兴趣的事件 + epoll_wait通知的事件]
 * 事件分发器Demultiplex模块的实现
 */

namespace zfwmuduo
{

} // namespace zfwmuduo