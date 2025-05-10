#include "AsyncLogging.h"
#include "../net/Buffer.h"
#include "Timestamp.h" // now()
#include "LogFile.h"
#include <cstring> // strlen()

namespace zfwmuduo
{
  AsyncLogging::AsyncLogging(const std::string &basename, off_t rollSize, int flushInterval)
      : mutex_(),
        cond_(mutex_),
        currentBuffer_(new Buffer),
        nextBuffer_(new Buffer),
        buffers_(),
        running_(false),
        basename_(basename),
        rollSize_(rollSize),
        flushInterval_(flushInterval),
        thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
        latch_(1)
  {
    buffers_.reserve(16);
  }

  void AsyncLogging::append(const char *logline, int len)
  {
    zfwmuduo::MutexLockGuard lock(mutex_);
    if (currentBuffer_->writeableBytes() > len)
    {
      currentBuffer_->append(logline, len);
    }
    else
    { // currentBuffer_满了,将其移入buffers_中, 并发现下一个空闲缓冲
      buffers_.push_back(std::move(currentBuffer_));

      if (nextBuffer_)
      {
        currentBuffer_ = std::move(nextBuffer_);
      }
      else
      {
        // 分配一个新的缓冲区
        currentBuffer_.reset(new Buffer); // Rarely happens (unique_ptr的reset)
      }
      currentBuffer_->append(logline, len);
      cond_.notify();
    }
  }

  void AsyncLogging::threadFunc()
  {
    latch_.countDown();
    LogFile output(basename_, rollSize_);
    BufferPtr newBuffer1(new Buffer);
    BufferPtr newBuffer2(new Buffer);
    BufferVector buffersToWrite;

    buffersToWrite.reserve(16); // 提前为 std::vector 预留指定数量的元素空间
    while (running_)
    {
      // TAG: 核心！负责处理日志缓冲区之间的数据传输和线程同步
      {
        zfwmuduo::MutexLockGuard lock(mutex_);
        if (buffers_.empty())
        { // 表示从未使用
          cond_.waitForSeconds(flushInterval_);
        }
        buffers_.push_back(std::move(currentBuffer_));
        currentBuffer_ = std::move(newBuffer1);
        buffersToWrite.swap(buffers_);
        if (!nextBuffer_)
        {
          nextBuffer_ = std::move(newBuffer2);
        }
      }

      // 日志丢弃机制:
      if (buffersToWrite.size() > 25)
      {
        char buf[256];
        snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n",
                 zfwmuduo::Timestamp::now().toString().c_str(),
                 buffersToWrite.size() - 2);

        output.append(buf, static_cast<int>(strlen(buf)));
        // 仅保留前两个缓冲区
        buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
      }

      for (const auto &buffer : buffersToWrite)
      {
        output.append(buffer->peek(), buffer->readableBytes());
      }

      // 缓冲区清理:
      //  将buffersToWrite内buffer重新填充newBuffer1和newBuffer2,
      //  这样下次执行时还有两个空闲buffer可用于替换前端的当前缓冲和预备缓冲
      if (buffersToWrite.size() > 2)
      {
        buffersToWrite.resize(2);
      }
      if (!newBuffer1)
      {
        newBuffer1 = std::move(buffersToWrite.back());
        buffersToWrite.pop_back();
        newBuffer1->reset(); // 避免新数据混淆, 重置缓冲区
      }
      if (!newBuffer2)
      {
        newBuffer2 = std::move(buffersToWrite.back());
        buffersToWrite.pop_back();
        newBuffer2->reset();
      }

      buffersToWrite.clear();
      output.flush();
    }
    output.flush();
  }
} // namespace zfwmuduo