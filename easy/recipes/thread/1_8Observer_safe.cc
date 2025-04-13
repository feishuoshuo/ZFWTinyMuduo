#include "../../../base/Mutex.h"
#include "../../../base/noncopyable.h"

#include <memory> // enable_shared_from_this<T>、weak_ptr、shared_ptr
#include <vector>

class Observable;

class Observer : public std::enable_shared_from_this<Observer>
{
public:
  virtual ~Observer() { /*subject_->unregister(this);*/ }
  virtual void update() = 0;

  void observe(Observable *s) // 下面两句不能合并！！
  {
    s->register_(shared_from_this()); // 将当前观察者对象注册到可观察对象 s 中
    subject_ = s;                     // 将可观察对象的指针保存到观察者对象中
  }

protected:
  Observable *subject_;
};

// void Observable::unregister(boost::weak_ptr<Observer> x)
//{
//   Iterator it = std::find(observers_.begin(), observers_.end(), x);
//   observers_.erase(it);
// }

// not 100% thread safe
class Observable
{
public:
  void register_(std::weak_ptr<Observer> x)
  {
    observers_.push_back(x);
  }
  // void unregister(weak_ptr<Observer>x); // 不需要它了
  void notifyObservers()
  {
    zfwmuduo::MutexLockGuard lock(mutex_);
    Iterator it = observers_.begin();
    while (it != observers_.end())
    {
      // TAG: 要在多个线程中同时访问一个shared_ptr,正确做法是用mutex保护
      std::shared_ptr<Observer> obj(it->lock());
      if (obj)
      {
        obj->update();
        ++it;
      }
      else
      {
        printf("notifyObservers() erase\n");
        it = observers_.erase(it);
      }
    }
  }

private:
  mutable zfwmuduo::MutexLock mutex_;
  // TODO：为什么不用shared_ptr
  std::vector<std::weak_ptr<Observer>> observers_;
  typedef std::vector<std::weak_ptr<Observer>>::iterator Iterator;
};

// ---------------------
class Foo : public Observer
{
  virtual void update()
  {
    printf("Foo::update() %p\n", this);
  }
};

int main()
{
  Observable subject;
  {
    std::shared_ptr<Foo> p(new Foo);
    p->observe(&subject);
    subject.notifyObservers();
  }
  subject.notifyObservers();
}