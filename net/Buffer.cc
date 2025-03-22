#include "Buffer.h"
#include <errno.h>
#include <sys/uio.h> // ::readv()
#include <unistd.h>  // ::write()

namespace zfwmuduo
{
  /**
   * 从fd上读取数据  Poller工作在LT模式
   * Buffer缓冲区是有大小的！！但从fd上读数据时，却不知道tcp数据最终的大小
   */
  ssize_t Buffer::readFd(int fd, int *saveErrno)
  {
    char extrabuf[65536] = {0}; // 64K 栈上的内存空间!!注意为什么！！栈分配空间快且编译器自动回收！
    // NOTE: iovec 是一个在 POSIX 标准中定义的结构体，用于表示分散/聚合（scatter/gather）I/O 操作中的内存区域。
    /**
     * 它通常用于高效的 I/O 操作，比如 readv() 和 writev()，
     * 这些函数允许程序一次性从多个内存区域读取或写入数据，
     * 而不需要先将数据拷贝到一个连续的缓冲区中
     */
    struct iovec vec[2];
    const size_t writable = writeableBytes(); // 这是Buffer底层缓冲区剩余的可写空间大小
    // 第一块缓冲区
    vec[0].iov_base = begin() + writeIndex_;
    vec[0].iov_len = writable;

    // 第二块缓冲区(如果上面填满, 会将余下的自动填入当中)
    vec[1].iov_base = extrabuf;
    vec[0].iov_len = sizeof extrabuf;

    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if (n < 0)
    {
      *saveErrno = errno;
    }
    else if (n <= writable) // Buffer的科协缓冲区已经够存储读出来的数据了
    {
      writeIndex_ += n;
    }
    else // extrabuf里面也写入了数据
    {
      writeIndex_ = buffer_.size();
      append(extrabuf, n - writable); // writeIndex_开始写 n-writable大小的数据
    }
    return n;
  }

  ssize_t Buffer::writeFd(int fd, int *saveErrno)
  {
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0)
    {
      *saveErrno = errno;
    }
    return n;
  }
} // namespace zfwmuduo