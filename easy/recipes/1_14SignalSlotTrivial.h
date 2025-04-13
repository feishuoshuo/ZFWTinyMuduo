#pragma once

#include <memory>
#include <vector>
/**
 * SignalTrivial 信号
 */

template <typename Signature>
class SignalTrivial;

// 这里对SignalTrivial进行了特化
// NOTE: 专门处理函数签名的形式为RET(ARGS...)的情况。RET表示返回值类型，ARGS...表示参数包（可变参数模板）
template <typename RET, typename... ARGS>
class SignalTrivial<RET(ARGS...)>
{
public:
  typedef std::function<void(ARGS...)> Functor;

  void connect(Functor &&func)
  {
    // NOTE: std::forward回顾知识点 完美转发、std::remove_reference
    functors_.push_back(std::forward<Functor>(func));
  }

  // 当信号被触发时，调用所有连接到该信号的槽函数
  void call(ARGS &&...args)
  {
    // gcc 4.6 supports
    // for (const Functor& f: functors_)
    typename std::vector<Functor>::iterator it = functors_.begin();
    for (; it != functors_.end(); ++it)
    {
      // 对每个槽函数(*it)，使用参数包展开args...调用它
      (*it)(args...);
    }
  }

private:
  std::vector<Functor> functors_;
};