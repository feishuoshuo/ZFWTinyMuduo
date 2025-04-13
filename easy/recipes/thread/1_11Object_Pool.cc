#include "../../../base/Mutex.h"
#include "../../../base/noncopyable.h"

#include <stdio.h>
#include <unordered_map>
#include <string>
#include <memory>     // enable_shared_from_this<T>、weak_ptr、shared_ptr
#include <functional> //bind

/**
 * 代表一只股票的价格.每只股票有唯一的字符串标识
 * 同一程序中每只出现的股票只有一个Stock对象，若多处用到同只股票，则Stock对象应被共享
 */
class Stock : zfwmuduo::noncopyable
{
public:
  Stock(const std::string &name)
      : name_(name)
  {
    printf(" Stock[%p] %s\n", this, name_.c_str());
  }

  ~Stock()
  {
    printf("~Stock[%p] %s\n", this, name_.c_str());
  }
  const std::string &key() const { return name_; }

private:
  std::string name_;
};

namespace version1
{
  // questionable code
  class StockFactory : zfwmuduo::noncopyable
  {
  public:
    std::shared_ptr<Stock> get(const std::string &key)
    {
      zfwmuduo::MutexLockGuard lock(mutex_);
      std::shared_ptr<Stock> &pStock = stocks_[key]; // 问题1:shared_ptr-->循环引用问题-->轻微内存泄漏
      if (!pStock)
      {
        pStock.reset(new Stock(key));
      }
      return pStock;
    }

  private:
    mutable zfwmuduo::MutexLock mutex_;
    std::unordered_map<std::string, std::shared_ptr<Stock>> stocks_;
  };
}

namespace version2 /* 依旧有轻微内存泄漏问题,因为stocks_只增不减 */
{
  class StockFactory : zfwmuduo::noncopyable
  {
  public:
    std::shared_ptr<Stock> get(const std::string &key)
    {
      std::shared_ptr<Stock> pStock;
      zfwmuduo::MutexLockGuard lock(mutex_);
      std::weak_ptr<Stock> &wkStock = stocks_[key]; // 解决1:改成weak_ptr
      pStock = wkStock.lock();
      if (!pStock)
      {
        pStock.reset(new Stock(key));
        wkStock = pStock;
      }
      return pStock;
    }

  private:
    mutable zfwmuduo::MutexLock mutex_;
    std::unordered_map<std::string, std::weak_ptr<Stock>> stocks_;
  };
}

namespace version3 /* L94存在线程安全问题,若StockFactory先于Stock对象析构,则会core dump */
{
  class StockFactory : zfwmuduo::noncopyable
  {
  public:
    std::shared_ptr<Stock> get(const std::string &key)
    {
      std::shared_ptr<Stock> pStock;
      zfwmuduo::MutexLockGuard lock(mutex_);
      std::weak_ptr<Stock> &wkStock = stocks_[key]; // 解决1:改成weak_ptr
      pStock = wkStock.lock();
      if (!pStock)
      {
        // 解决2
        //  StockFactory生命周期若短于Stock对象则Stock析构时去回调其deleteStock就会core dump
        pStock.reset(new Stock(key),
                     bind(&StockFactory::deleteStock,
                          this, std::placeholders::_1));
        wkStock = pStock;
      }
      return pStock;
    }

  private:
    void deleteStock(Stock *stock)
    {
      printf("deleteStock[%p]\n", stock);
      if (stock)
      {
        zfwmuduo::MutexLockGuard lock(mutex_);
        stocks_.erase(stock->key());
      }
      delete stock; // sorry, I lied
    }

    mutable zfwmuduo::MutexLock mutex_;
    std::unordered_map<std::string, std::weak_ptr<Stock>> stocks_;
  };
}

/* L94存在线程安全问题,若StockFactory先于Stock对象析构,则会core dump */
class StockFactory : public std::enable_shared_from_this<StockFactory>,
                     zfwmuduo::noncopyable
{
public:
  std::shared_ptr<Stock> get(const std::string &key)
  {
    std::shared_ptr<Stock> pStock;
    zfwmuduo::MutexLockGuard lock(mutex_);
    std::weak_ptr<Stock> &wkStock = stocks_[key]; /*注意wkStock是引用*/ // 解决1:改成weak_ptr
    // NOTE: std::weak_ptr 提供了 lock() 方法，用于尝试获取一个指向对象的 std::shared_ptr。
    /**
     * 如果对象仍然存在，lock() 会返回一个有效的 std::shared_ptr；
     * 如果对象已经被销毁，lock() 会返回一个空的 std::shared_ptr
     */
    pStock = wkStock.lock();
    if (!pStock)
    {
      // 解决3 this改成shared_from_this()【问题:这样似乎将StockFactory生命周期意外延长了】
      // pStock.reset(new Stock(key),
      //              bind(&StockFactory::deleteStock,
      //                   shared_from_this(), std::placeholders::_1));

      // 解决4:将shared_from_this()强转成weak_ptr才不会延长其寿命 + 使用弱回调技术
      // reset第二个参数为一个自定义删除器
      pStock.reset(new Stock(key),
                   bind(&StockFactory::weakDeleteCallback,
                        std::weak_ptr<StockFactory>(shared_from_this()), std::placeholders::_1));

      wkStock = pStock;
    }
    return pStock;
  }

private:
  /**
   * 如果weakDeleteCallback是一个普通成员函数，std::bind 生成的可调用对象将需要一个StockFactory对象实例来调用它。
   * 然而，shared_ptr的自定义删除器 无法提供这样的对象实例，因为它只是一个简单的函数调用。
   * 因此，weakDeleteCallback必须是静态成员函数，这样它就可以直接通过类名调用，而不需要任何对象实例。
   */
  static void weakDeleteCallback(const std::weak_ptr<StockFactory> &wkFactory, Stock *stock)
  {
    printf("deleteStock[%p]\n", stock);
    std::shared_ptr<StockFactory> factory(wkFactory.lock());
    if (factory)
    {
      factory->removeStock(stock);
    }
    else
    {
      printf("factory died.\n");
    }
    delete stock; // sorry, I lied
  }

  void removeStock(Stock *stock)
  {
    if (stock)
    {
      zfwmuduo::MutexLockGuard lock(mutex_);
      auto it = stocks_.find(stock->key());
      if (it != stocks_.end() && it->second.expired()) // expired()用于检查 weak_ptr是否已经过期
      {
        stocks_.erase(stock->key());
      }
    }
  }

  mutable zfwmuduo::MutexLock mutex_;
  std::unordered_map<std::string, std::weak_ptr<Stock>> stocks_;
};

void testLongLifeFactory()
{
  std::shared_ptr<StockFactory> factory(new StockFactory);
  {
    std::shared_ptr<Stock> stock = factory->get("NYSE:IBM");
    std::shared_ptr<Stock> stock2 = factory->get("NYSE:IBM");
    assert(stock == stock2);
    // stock destructs here
  }
  // factory destructs here
}

void testShortLifeFactory()
{
  std::shared_ptr<Stock> stock;
  {
    std::shared_ptr<StockFactory> factory(new StockFactory);
    stock = factory->get("NYSE:IBM");
    std::shared_ptr<Stock> stock2 = factory->get("NYSE:IBM");
    assert(stock == stock2);
    // factory destructs here
  }
  // stock destructs here
}
int main()
{
  version1::StockFactory sf1;
  version2::StockFactory sf2;
  version3::StockFactory sf3;
  std::shared_ptr<version3::StockFactory> sf4(new version3::StockFactory);
  std::shared_ptr<StockFactory> sf5(new StockFactory);

  {
    std::shared_ptr<Stock> s1 = sf1.get("stock1");
  }

  {
    std::shared_ptr<Stock> s2 = sf2.get("stock2");
  }

  {
    std::shared_ptr<Stock> s3 = sf3.get("stock3");
  }

  {
    std::shared_ptr<Stock> s4 = sf4->get("stock4");
  }

  {
    std::shared_ptr<Stock> s5 = sf5->get("stock5");
  }

  testLongLifeFactory();
  testShortLifeFactory();
}