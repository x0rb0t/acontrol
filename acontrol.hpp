#ifndef ACONTROL_HPP
#define ACONTROL_HPP
#include <thread>
#include <tuple>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <deque>
#include <vector>
namespace act
{
  class abstract_task
  {
  protected:
    std::mutex m_mutex;
    std::condition_variable m_cond_var;
    bool m_complete;
  public:
    abstract_task(): m_complete(false) {}
    virtual ~abstract_task() {}
    virtual void invoke() = 0;
    virtual void wait()
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
    volatile bool m_run;
  public:
    struct sync
    {

    };

    control(std::size_t pool_size = 2)
    {
      m_run = true;
      auto func = [this]()
      {
        while (m_run)
        {
          std::unique_lock<std::mutex> lock(m_mutex);
          if (m_tasks.empty())
          {
            m_cond_var.wait(lock);
          }
          else
          {
            std::shared_ptr<abstract_task> t = m_tasks.front();
            m_tasks.pop_front();
            lock.unlock();
            t->invoke();
            lock.lock();
            m_empty_cond.notify_all();
          }
        }
      };
      m_pool.resize(pool_size > 0 ? pool_size : 1);
      for(auto i = m_pool.begin(); i != m_pool.end(); ++i)
      {
        *i = new std::thread(func);
      }
    }
    ~control()
    {
      m_run = false;
      std::unique_lock<std::mutex> lock(m_mutex);
      m_cond_var.notify_all();
      lock.unlock();
      for(auto i = m_pool.begin(); i != m_pool.end(); ++i)
      {
        (*i)->join();
        delete *i;
      }
    }
    control & operator << (const std::shared_ptr<abstract_task> & t)
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_tasks.push_back(t);
      m_cond_var.notify_one();
      return *this;
    }
    control & operator<< (const sync & )
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      while (!m_tasks.empty())
      {
        m_empty_cond.wait(lock);
      }
      return *this;
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

    public:
    template <typename ... Args>
    task(const T &v, Args... args) {
      this->m_func = &v;
      this->m_vars = std::make_tuple(args ...);
    }
    virtual ~task() {}
  };

  template <typename ClassType, typename ReturnType, typename ... Args>
  class task<ReturnType(ClassType::*)(Args...) const>: public abstract_task
  {
  protected:
    const ClassType *m_func;
    std::tuple<Args...> m_vars;
    ReturnType m_return;
  public:
    virtual ~task() {}
    virtual void invoke()
    {
      ReturnType r = caller(typename gens<sizeof...(Args)>::type());
      m_mutex.lock();
      m_return = r;
      m_complete = true;
      m_cond_var.notify_all();
      m_mutex.unlock();
    }
    template<int ...S>
    ReturnType caller(seq<S...>) {
       return m_func->operator()(std::get<S>(m_vars) ...);
    }
    ReturnType get()
    {
      wait();
      return m_return;
    }
  };


  template <typename T, typename ... Args>
  std::shared_ptr<task<T>> make_task(T func, Args ... args )
  {
    return std::shared_ptr<task<T>>(new task<T>(func, args ...));
  }

}
#endif // ACONTROL_HPP
