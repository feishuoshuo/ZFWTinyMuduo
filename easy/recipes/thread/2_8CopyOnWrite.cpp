#include "../../../base/Mutex.h"
#include "../../../base/Thread.h"
#include <vector>
#include <assert.h>

class Foo
{
public:
  void doit() const;
};

typedef std::vector<Foo> Foolist;
typedef std::shared_ptr<Foolist> FoolistPtr;
FoolistPtr g_foos;
zfwmuduo::MutexLock mutex;

void post(const Foo &f)
{
  printf("post\n");
  zfwmuduo::MutexLockGuard lock(mutex);
  if (!g_foos.unique())
  { /**
     * 检查智能指针是否唯一
     * 不是唯一, 则表示别的线程正在读取Foolist,
     * 我们不能原地修改，而是复制一份, 在副本上修改
     */
    g_foos.reset(new Foolist(*g_foos));
    printf("copy the whole list\n");
  }
  assert(g_foos.unique());
  g_foos->push_back(f);
}

void traverse()
{
  FoolistPtr foos;
  {
    zfwmuduo::MutexLockGuard lock(mutex);
    foos = g_foos; // 使得引用计数+1
    assert(!g_foos.unique());
  }

  for (std::vector<Foo>::const_iterator it = foos->begin();
       it != foos->end(); ++it)
  {
    it->doit();
  }
}

void Foo::doit() const
{
  Foo f;
  post(f);
}

int main()
{
  g_foos.reset(new Foolist);
  Foo f;
  post(f);
  traverse();
}