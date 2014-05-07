#include <iostream>
#include <acontrol.hpp>
using namespace std;
int main()
{
  act::control control(8);  
  std::atomic<int> cnt = {0};
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
  for (int i =0; i < 100; ++i)
  {
    task_type task = act::make_task(lmdb, big.begin(), big.end());
    control << task;
    tasks.push_back(task);
  } 
  cout << "wait! " << endl;
  //control << act::control::sync();

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
  cout << "wait2 " << endl;
  control << act::control::sync();
  cout << "Hello World: " << end_task->get() << endl;

  return 0;
}

