#pragma once

#include <vector>
#include <string>
#include <algorithm> // copy()
#include "../base/noncopyable.h"

/**
 * 网络库底层的缓冲区类型定义
 */

namespace zfwmuduo
{

  /// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
  ///
  /// @code
  /// +---------------------------+------------------+------------------+
  /// | prependable bytes(8Byte)  |  readable bytes  |  writable bytes  |
  /// |[主要用来解决粘包问题]       |     (CONTENT)    |                  |
  /// +----------------------------+------------------+------------------+
  /// |                           |                  |                  |
  /// 0      <=              readerIndex   <=    writerIndex    <=     size
  /// @endcode
  class Buffer : noncopyable
  {

  public:
    static const size_t kCheapPrepend = 8;   // 前面记录数据包的长度 prependable bytes
    static const size_t kInitialSize = 1024; // 后边缓冲区的大小

    explicit Buffer(size_t initialSize = kInitialSize) : buffer_(kCheapPrepend + initialSize),
                                                         readerIndex_(kCheapPrepend),
                                                         writeIndex_(kCheapPrepend) {}

    ~Buffer();

    size_t readableBytes() const { return writeIndex_ - readerIndex_; }
    size_t writeableBytes() const { return buffer_.size() - writeIndex_; }
    size_t prependableBytes() const { return readerIndex_; }

    // 返回缓冲区中, 可读数据的其实地址
    const char *peek() const { return begin() + readerIndex_; }

    // onMessage string <- Buffer
    void retrieve(size_t len)
    {
      if (len < readableBytes()) // 应用只读取了可读缓冲区数据的一部分
      {
        readerIndex_ += len;
      }
      else
      { // len == readableBytes()
        retrieveAll();
      }
    }
    void retrieveAll()
    {
      readerIndex_ = writeIndex_ = kCheapPrepend;
    }

    // 把onMessage函数上报的Buffer数据，转成string类型的数据返回
    std::string retrieveAllAsString()
    {
      return retrieveAsString(readableBytes()); // 应用可读取的长度
    }
    std::string retrieveAsString(size_t len)
    {
      std::string result(peek(), len); // 读取出缓冲区的可读数据
      retrieve(len);                   // 对缓冲区进行复位操作
      return result;
    }

    // buffer_.size() - writeIndex_    len
    // 确保读缓冲区空间足够
    void ensureWritableBytes(size_t len)
    {
      // 缓冲区不足-->扩容
      if (writeableBytes() < len)
        makeSpace(len);
    }

    // 将[data, data+len]内存上的数据，添加到writable缓冲区当中
    void append(const char *data, size_t len)
    {
      ensureWritableBytes(len);
      std::copy(data, data + len, beginWrite());
      writeIndex_ += len;
    }

    char *beginWrite(size_t len)
    {
      return begin() + writeIndex_;
    }

    const char *beginWrite(size_t len) const
    {
      return begin() + writeIndex_;
    }

    // TAG: [值得借鉴] 从fd上读取数据
    ssize_t readFd(int fd, int *saveErrno);

    // 通过fd发送数据
    ssize_t writeFd(int fd, int *saveErrno);

  private:
    char *begin()
    {
      // vector底层数组首元素的地址, 即数组的起始地址
      //  1. it.operator*()取值   2.operator&()取地址
      return &*buffer_.begin();
    }
    const char *begin() const { return &*buffer_.begin(); }
    char *beginWrite() { return begin() + writeIndex_; }
    const char *beginWrite() const { return begin() + writeIndex_; }

    // TAG: 扩容函数
    void makeSpace(size_t len)
    {
      /**
       * kCheapPrepend  |-----|   reader      |      writer       |
       *                      ^               ^
       *                 readerIndex      writerIndex
       * [ prependableBytes() ]               [ writeableBytes() ]
       * kCheapPrepend  |                   len                        |
       */
      if (writeableBytes() + prependableBytes() < len + kCheapPrepend) // 可写部分 + 空闲部分 < 期待长度len + 首部长
      {
        buffer_.resize(writeIndex_ + len);
      }
      else
      { // 将剩余读缓冲区(还未读部分)的数据区域挪到空闲部分(已读部分), 给写缓冲区腾位置 (也就是将已读部分区域腾掉)
        size_t readable = readableBytes();
        std::copy(begin() + readerIndex_,
                  begin() + writeIndex_,
                  begin() + kCheapPrepend);
        readerIndex_ = kCheapPrepend;
        writeIndex_ = readerIndex_ + readable;
      }
    }

    std::vector<char> buffer_;
    size_t readerIndex_; // 数据可读下标
    size_t writeIndex_;  // 数据可写下标
  };

} // namespace zfwmuduo