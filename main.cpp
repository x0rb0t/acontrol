#include <iostream>
#include <acontrol.hpp>
using namespace std;

int main()
{
  act::control control(8);
  for (int i =0; i < 100; ++i)
  {
    auto lmdb = [](){ std::this_thread::sleep_for(std::chrono::seconds(1)); cout << "q" << endl; return 0;};

    control << act::make_task(lmdb );
    control << act::make_task(lmdb );
    control << act::make_task(lmdb );
  }
  cout << "wait!" << endl;
  control << act::control::sync();

  cout << "Hello World!" << endl;
  return 0;
}

