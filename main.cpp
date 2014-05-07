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
  std::vector<decltype(act::make_task(lmdb, big.begin(), big.end()))> tasks;
  for (int i =0; i < 100; ++i)
  {
    auto task = act::make_task(lmdb, big.begin(), big.end());
    control << task;
    tasks.push_back(task);
  } 
  cout << "wait! " << endl;
  //control << act::control::sync();
  double s = 0;
  for (auto x: tasks)
  {
    s += x->get();
    cout << s << endl;
  }
  cout << "Hello World: " << s << endl;

  return 0;
}

