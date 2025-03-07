#pragma once
// NOTE:#pragma once 是一种在 C 和 C++ 编程中常用的预处理指令，用于防止头文件（.h 或 .hpp 文件）被重复包含。它是一种简单且广泛使用的替代传统头文件保护宏的方法

/**
 * noncopyable被继承以后，派生类对象可以正常构造析构，
 * 但是派生类对象无法进行拷贝构造和赋值操作
 */
namespace zfwmuduo
{
  // NOTE: noncopyable 是一个常见的 C++ 设计模式，用于创建一个不可拷贝的类。 它的作用是禁用类的拷贝构造函数和赋值操作符，从而防止对象被拷贝或赋值
  class noncopyable
  {
  protected:
    noncopyable() = default;
    ~noncopyable() = default;

  public:
    noncopyable(const noncopyable &) = delete;    // 将拷贝构造删除
    void operator=(const noncopyable &) = delete; // 将赋值操作删除
  };

} // namespace zfwmuduo