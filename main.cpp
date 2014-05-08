#include <iostream>
#include <acontrol.hpp>

#include <future>

#define N_SIZE 100000000
#define N_THREADS 7
#define N_PARALLEL 73

int RandomNumber () { return (std::rand()%100); }

auto lmdb = [](std::vector<double>::const_iterator begin, std::vector<double>::const_iterator end)
{
  double sum = 0;
  for (auto i = begin; i != end; ++i)
  {
    sum+=*i;
  }
  return sum;
};

double test_act(const std::vector<double> & input)
{
  act::control control(N_THREADS);

  typedef decltype(act::make_task(lmdb, input.begin(), input.end())) task_type;
  std::vector<task_type> tasks;

  for (int i =0; i < N_PARALLEL; ++i)
  {
    task_type task = act::make_task(lmdb, input.begin(), input.end());
    control << task;
    tasks.push_back(task);
  }
  auto end_task = act::make_task([](std::vector<task_type>::const_iterator begin, std::vector<task_type>::const_iterator end)
  {
    double sum = 0;
    for (auto i = begin; i != end; ++i)
    {
      sum+=(*i)->get();
    }
    return sum;
  }
  , tasks.begin(), tasks.end());
  control << end_task;
  control << act::control::sync();
  return end_task->get();
}

double test_async(const std::vector<double> & input)
{
  typedef decltype(std::async(std::launch::async, lmdb, input.begin(), input.end())) async_type;
  std::vector<async_type> asyncs;
  for (int i =0; i < N_PARALLEL; ++i)
  {
    asyncs.push_back(std::move(std::async(std::launch::async, lmdb, input.begin(), input.end())));
  }
  auto end_async = std::async(std::launch::async, [](std::vector<async_type>::iterator begin, std::vector<async_type>::iterator end)
  {
    double sum = 0;
    for (auto i = begin; i != end; ++i)
    {
      sum+=(*i).get();
    }
    return sum;
  }
  , asyncs.begin(), asyncs.end());
  return end_async.get();
}

int main()
{  
  typedef double(*p_test)(const std::vector<double> &);
  std::vector<double> INPUT(N_SIZE);
  std::generate (INPUT.begin(), INPUT.end(), RandomNumber);
  std::vector<std::tuple<std::string, p_test>> tests =
  {
    std::make_tuple(std::string("test_act"), test_act),
    std::make_tuple(std::string("test_async"), test_async)
  };
  double test_val = lmdb(INPUT.begin(), INPUT.end()) * N_PARALLEL;
  for (auto test: tests)
  {
    std::string name;
    p_test func;
    std::tie(name,func) = test;
    double time = 0;
    bool v = true;
    int i = 0;
    for (i = 0; i < 10; ++i)
    {
      auto t1 = std::chrono::high_resolution_clock::now();
      double res = func(INPUT);
      auto t2 = std::chrono::high_resolution_clock::now();
      time += std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count();
      if (res != test_val)
      {
        v = false;
        break;
      }
    }
    std::cout << name
         << " "
         << (time/i)
         << " " << (v ? "OK" : "ERR") << std::endl;
  }
  return 0;
}

