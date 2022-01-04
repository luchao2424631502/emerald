#ifndef _OSSQUEUE_H_
#define _OSSQUEUE_H_

#include <queue>
#include <boost/thread.hpp>
#include <boost/thread.hpp/thread_time.hpp>

#include "core.h"

template<typename T>
class ossQueue
{
  private:
    std::queue<T> _queue;
    boost::mutex _mutex;
    boost::condition_variable _cond;
  public:
    unsigned int size() 
    {
      //RAII作用域锁,离开scope析构就释放
      boost::mutex::scoped_lock lock(_mutex);
      return (unsigned int)_queue.size();
    }

    //唤醒 wait_pop()一定要得到元素的 线程
    void push(const T& value)
    {
      boost::mutex::scoped_lock lock(_mutex);
      _queue.push(value);
      lock.unlock();
      _cond.notify_one(); //
    }

    bool empty() 
    {
      boost::mutex::scoped_lock lock(_mutex);
      return _queue.empty();
    }

    //不关注queue中是否有元素
    bool try_pop(T &value)
    {
      boost::mutex::scoped_lock lock(_mutex);
      if (_queue.empty())
        return false;
      value = _queue.front();
      _queue.pop();
      return true;
    }

    //等待(queue可能没有元素) 然后pop
    void wait_and_pop(T &value)
    {
      boost::mutex::scoped_lock lock(_mutex);
      while (_queue.empty())
      {
        //wait的过程中,先release lock,线程阻塞,
        //当被notify(通知)线程唤醒会重新拿到lock,执行
        //对比mutex,while()轮旋的次数减少了好多
        _cond.wait(lock);
      }
      value = _queue.front();
      _queue.pop();
    }

    //限时等待 
    bool time_wait_and_pop(T &value,long long millsec)
    {
      //typedef boost::posix_time::ptime system_time;
      //https://www.boost.org/doc/libs/1_40_0/doc/html/thread/time.html
      boost::system_time const timeout=boost::get_system_time() + boost::posix_time::milliseconds(millsec);

      boost::mutex::scoped_lock lock(_mutex);
      while (_queue.empty())  //限时等待条件变量被通知
      {
        //等待超时,则失败
        if (!_cond.time_wait(lock,timeout))
          return false;
        else //没有这句也行,再次while empty()肯定是false,也退出了
          break;
      }
      //等到了一次push,
      value = _queue.front();
      _queue.pop();
      return true;
    }
};

#endif
