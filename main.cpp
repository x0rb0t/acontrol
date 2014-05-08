#include <iostream>
#include <acontrol.hpp>

#include <future>
using namespace std;
int main()
{
  act::control control(6);
  //std::atomic<int> cnt = {0};
  std::vector<double> big;
  for (int i = 0; i < 100000000; ++i)
  {
    big.push_back(i);
  }
  auto lmdb = [](std::vector<double>::iterator begin, std::vector<double>::iterator end)
  {
    double sum = 0;
    for (auto i = begin; i != end; ++i)
    {
      sum+=*i;
    }
    return sum;
  };
  typedef decltype(act::make_task(lmdb, big.begin(), big.end())) task_type;
  std::vector<task_type> tasks;

  auto t1 = std::chrono::high_resolution_clock::now();
  for (int i =0; i < 100; ++i)
  {
    task_type task = act::make_task(lmdb, big.begin(), big.end());
    control << task;
    tasks.push_back(task);
  }
  auto end_task = act::make_task([](std::vector<task_type>::iterator begin, std::vector<task_type>::iterator end)
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
  double val = end_task->get();
  auto t2 = std::chrono::high_resolution_clock::now();
  cout << "act: " << std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count() << " " << val << endl;

  t1 = std::chrono::high_resolution_clock::now();
  typedef decltype(std::async(std::launch::async, lmdb, big.begin(), big.end())) async_type;
  std::vector<async_type> asyncs;
  for (int i =0; i < 100; ++i)
  {
    //async_type handle = ;
    asyncs.push_back(std::move(std::async(std::launch::async, lmdb, big.begin(), big.end())));
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
  val = end_async.get();
  t2 = std::chrono::high_resolution_clock::now();
  cout << "async: " << std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count() << " " << val << endl;

  return 0;
}

