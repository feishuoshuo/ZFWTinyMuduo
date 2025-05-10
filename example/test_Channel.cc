#include "../base/CurrentThread.h"
#include "../net/EventLoop.h"
#include "../net/TimerQueue.h"
#include "../base/Thread.h"
#include "../net/EventLoopThread.h"
#include <gtest/gtest.h>
#include <atomic>
#include <stdio.h>
#include <memory>
#include <functional> // bind()

/**
 * 3-用Channel关注timerfd的可读事件
 *
 * 4-TimerQueue示例
 * 7-Acceptor示例
 * 8-discard服务
 * 9-echo服务
 * 10-发送两次数据，测试TcpConnection::send()
 * 11-chargen服务,使用WriteCompleteCallback
 * 12-Connerctor示例
 * 13-TcpClient示例
 */