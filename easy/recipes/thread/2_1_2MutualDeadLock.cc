#include "../../../base/Mutex.h"
#include "../../../base/Thread.h"
#include <set>
#include <stdio.h>

class Request;

class Inventory
{
public:
  void add(Request *req)
  {
    zfwmuduo::MutexLockGuard lock(mutex_);
    requests_.insert(req);
  }

  void remove(Request *req) __attribute__((noinline))
  {
    zfwmuduo::MutexLockGuard lock(mutex_);
    requests_.erase(req);
  }

  void printAll() const;

private:
  mutable zfwmuduo::MutexLock mutex_;
  std::set<Request *> requests_;
};

Inventory g_inventory;

class Request
{
public:
  void process() // __attribute__ ((noinline))
  {
    zfwmuduo::MutexLockGuard lock(mutex_);
    g_inventory.add(this);
    // ...
  }

  ~Request() __attribute__((noinline))
  {
    zfwmuduo::MutexLockGuard lock(mutex_);
    sleep(1);
    g_inventory.remove(this);
  }

  void print() const __attribute__((noinline))
  {
    zfwmuduo::MutexLockGuard lock(mutex_);
    // ...
  }

private:
  mutable zfwmuduo::MutexLock mutex_;
};

void Inventory::printAll() const
{
  zfwmuduo::MutexLockGuard lock(mutex_);
  sleep(1); // 为了容易复现死锁, 这里用了延时
  for (std::set<Request *>::const_iterator it = requests_.begin();
       it != requests_.end();
       ++it)
  {
    (*it)->print();
  }
  printf("Inventory::printAll() unlocked\n");
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
