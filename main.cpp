#include <iostream>
#include <acontrol.hpp>
using namespace std;

int main()
{
  act::control control(8);
  for (int i =0; i < 100; ++i)
  {
    auto lmdb = [](int j){ std::this_thread::sleep_for(std::chrono::seconds(1)); cout << "q " << j << endl;};

    control << act::make_task(lmdb, 1);
    control << act::make_task(lmdb, 2);
    control << act::make_task(lmdb, 3);
  }
  cout << "wait!" << endl;
  control << act::control::sync();

  cout << "Hello World!" << endl;
  return 0;
}

