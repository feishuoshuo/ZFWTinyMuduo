#include "../../../base/Mutex.h"
#include "../../../base/Thread.h"
#include <set>
#include <stdio.h>
#include <memory>
#include <assert.h>

class Request;

class Inventory
{
public:
  Inventory() : requests_(new RequestList) {}

  void add(Request *req)
  {
    zfwmuduo::MutexLockGuard lock(mutex_);
    if (!requests_.unique())
    {
      // reset释放当前对象, 开始管理新的对象
      requests_.reset(new RequestList(*requests_));
      printf("Inventory::add() copy the whole list\n");
    }
    assert(requests_.unique());
    requests_->insert(req);
  }

  void remove(Request *req) // __attribute__((noinline))
  {
    zfwmuduo::MutexLockGuard lock(mutex_);
    if (!requests_.unique())
    {
      requests_.reset(new RequestList(*requests_));
      printf("Inventory::remove() copy the whole list\n");
    }
    assert(requests_.unique());
    requests_->erase(req);
  }

  void printAll() const;

private:
  typedef std::set<Request *> RequestList;
  typedef std::shared_ptr<RequestList> RequestListPtr;

  RequestListPtr getDate() const
  {
    zfwmuduo::MutexLockGuard lock(mutex_);
    return requests_;
  }

  mutable zfwmuduo::MutexLock mutex_;
  RequestListPtr requests_;
};

Inventory g_inventory;

class Request
{
public:
  Request() : x_(0) {}
  ~Request() __attribute__((noinline))
  {
    zfwmuduo::MutexLockGuard lock(mutex_);
    x_ = -1;
    sleep(1);
    g_inventory.remove(this);
  }

  void process() // __attribute__ ((noinline))
  {
    zfwmuduo::MutexLockGuard lock(mutex_);
    g_inventory.add(this);
    // ...
  }

  void print() const __attribute__((noinline))
  {
    zfwmuduo::MutexLockGuard lock(mutex_);
    // ...
    printf("print Request %p x=%d\n", this, x_);
  }

private:
  mutable zfwmuduo::MutexLock mutex_;
  int x_;
};

void Inventory::printAll() const
{
  zfwmuduo::MutexLockGuard lock(mutex_);
  sleep(1); // 为了容易复现死锁, 这里用了延时
  for (std::set<Request *>::const_iterator it = requests_->begin();
       it != requests_->end();
       ++it)
  {
    (*it)->print();
  }
  // printf("Inventory::printAll() unlocked\n");
}

/*
void Inventory::printAll() const
{
  std::set<Request*> requests
  {
    muduo::MutexLockGuard lock(mutex_);
    requests = requests_;
  }
  for (std::set<Request*>::const_iterator it = requests.begin();
      it != requests.end();
      ++it)
  {
    (*it)->print();
  }
}
*/

void threadFunc()
{
  Request *req = new Request;
  req->process();
  delete req;
}

void thread1()
{
  zfwmuduo::Thread thread(threadFunc);
  thread.start();
  usleep(500 * 1000); // 为了让另一个线程等在前面sleep()上
  g_inventory.printAll();
  thread.join();
}

void thread2()
{
  zfwmuduo::Thread thread(threadFunc);
  thread.start();
  usleep(500 * 1000); // 为了让另一个线程等在前面sleep()上
  g_inventory.printAll();
  thread.join();
}

int main()
{
  thread1();
  thread2();
}
