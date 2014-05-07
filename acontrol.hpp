#ifndef ACONTROL_HPP
#define ACONTROL_HPP
#include <thread>
#include <tuple>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <deque>
#include <vector>
#include <algorithm>
#include <atomic>
namespace act
{
  class abstract_task
  {
  protected:
    mutable std::mutex m_mutex;
    mutable std::condition_variable m_cond_var;
    mutable bool m_complete;
  public:
    abstract_task(): m_complete(false) {}
    virtual ~abstract_task() {}
    virtual void invoke() = 0;
    virtual void wait() const
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      while (!m_complete)
      {
        m_cond_var.wait(lock);
      }
    }
  };

  class control
  {
    std::deque<std::shared_ptr<abstract_task>> m_tasks;
    std::vector<std::thread*> m_pool;
    std::mutex m_mutex;
    std::condition_variable m_cond_var;
    std::condition_variable m_empty_cond;
    std::atomic<bool> m_run;
    std::vector<bool> m_active;
  public:
    struct sync
    {

    };

    control(std::size_t pool_size = 2)
    {
      m_run.store(true, std::memory_order_relaxed);
      auto func = [this](int n)
      {
        while (m_run.load(std::memory_order_relaxed))
        {
          std::unique_lock<std::mutex> lock(m_mutex);
          m_active[n] = true;
          if (m_tasks.empty())
          {
            m_empty_cond.notify_all();
            m_active[n] = false;
            m_cond_var.wait(lock);
          }
          else
          {
            std::shared_ptr<abstract_task> t = m_tasks.front();
            m_tasks.pop_front();
            lock.unlock();
            t->invoke();
            lock.lock();
            m_active[n] = false;
          }
        }        
      };
      m_active.resize(pool_size > 0 ? pool_size : 1, false);
      m_pool.resize(pool_size > 0 ? pool_size : 1);
      int n = 0;
      for(auto i = m_pool.begin(); i != m_pool.end(); ++i, ++n)
      {
        *i = new std::thread(func, n);
      }
    }
    ~control()
    {
      m_run.store(false, std::memory_order_relaxed);
      std::unique_lock<std::mutex> lock(m_mutex);
      m_cond_var.notify_all();
      lock.unlock();
      for(auto i = m_pool.begin(); i != m_pool.end(); ++i)
      {
        (*i)->join();
        delete *i;
      }
    }

    control & schedule(const std::shared_ptr<abstract_task> & t)
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      if (t)
      {
        m_tasks.push_back(t);
        m_cond_var.notify_one();
      }
      return *this;
    }

    control & synchronize()
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      bool x = false;
      while (!m_tasks.empty() && (std::for_each(m_active.begin(), m_active.end(), [&x](bool e){if (e) x = true;}), x))
      {
        m_empty_cond.wait(lock);
        x = false;
      }
      return *this;
    }

    control & operator << (const std::shared_ptr<abstract_task> & t)
    {
      return schedule(t);
    }

    control & operator<< (const sync & )
    {
      return synchronize();
    }

  };


  template<int ...>
  struct seq { };

  template<int N, int ...S>
  struct gens : gens<N-1, N-1, S...> { };

  template<int ...S>
  struct gens<0, S...> {
    typedef seq<S...> type;
  };

  template <typename T>
  class task
      : public task<decltype(&T::operator())>
  {
  };

  template <typename ClassType, typename ReturnType, typename ... Args>
  class task<ReturnType(ClassType::*)(Args...) const>: public abstract_task
  {
  protected:
    const ClassType *m_func;
    std::tuple<Args...> m_vars;
    ReturnType m_return;
  public:
    task(const ClassType &v, Args... args): m_func(&v), m_vars(args ...)
    {
    }
    virtual ~task() {}
    virtual void invoke()
    {
      ReturnType r = caller(typename gens<sizeof...(Args)>::type());
      std::unique_lock<std::mutex> lock(m_mutex);
      m_return = r;
      m_complete = true;
      m_cond_var.notify_all();
    }
    ReturnType get() const
    {
      wait();
      return m_return;
    }

  private:
    template<int ...S>
    ReturnType caller(seq<S...>) const
    {
       return m_func->operator()(std::get<S>(m_vars) ...);
    }    
  };

  template <typename ClassType, typename ... Args>
  class task<void(ClassType::*)(Args...) const>: public abstract_task
  {
  protected:

    const ClassType *m_func;
    std::tuple<Args...> m_vars;
  public:
    task(const ClassType &v, Args... args): m_func(&v), m_vars(args ...)
    {
    }
    virtual ~task() {}
    virtual void invoke()
    {
      caller(typename gens<sizeof...(Args)>::type());
      std::unique_lock<std::mutex> lock(m_mutex);
      m_complete = true;
      m_cond_var.notify_all();
    }
  private:
    template<int ...S>
    void caller(seq<S...>) {
       m_func->operator()(std::get<S>(m_vars) ...);
    }
  };

  template <typename ClassType>
  class task<void(ClassType::*)(void) const>: public abstract_task
  {
  protected:
    const ClassType *m_func;
  public:
    task(const ClassType &v): m_func(&v)
    {
    }
    virtual ~task() {}
    virtual void invoke()
    {
      m_func->operator()();
      std::unique_lock<std::mutex> lock(m_mutex);
      m_complete = true;
      m_cond_var.notify_all();
    }
  };

  template <typename T, typename ... Args>
  std::shared_ptr<task<decltype(&T::operator())>> make_task(T func, Args ... args )
  {
    return std::shared_ptr<task<decltype(&T::operator())>>(new task<decltype(&T::operator())>(func, args ...));
  }


}
#endif // ACONTROL_HPP
