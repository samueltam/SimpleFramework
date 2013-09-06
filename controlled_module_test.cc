//controlled_module demo

#include <stdio.h>
#include <iostream>
#include "controlled_module.h"

using namespace std;

class thd: public controlled_module
{
  public:
    virtual bool initialize()
    {
      cout << "thd init" << endl;
      return true;
    }
    virtual void release()
    {
      cout << "thd release" << endl;
    }
    virtual bool work()
    {
      //your work........ 
      cout << "thd work" << endl;
      return controlled_module::work();
    }
};

int main(int argc, char* argv[])
{
  thd t;
  
  t.start();
  
  printf("input anything you like:\n");

  char buf[10];
  fgets(buf, sizeof(buf), stdin);
  
  t.stop();
  return 0;
}

