#include <stdio.h>
#include <iostream>

#include "controlled_module_ex.h"

using namespace std;

class thdex: public controlled_module_ex
{
  protected:
    virtual void message(const _command & cmd)
    {
      controlled_module_ex::message(cmd);
      if (cmd.nCmd == BM_USER+1)
      {
        cout << "get message" << endl;
      }
    }
};

int main(int argc, char* argv[])
{
  thdex t;
  
  t.safestart();
  t.postmessage(BM_USER+1);
  
  printf("input anything you like:\n");

  char buf[10];
  fgets(buf, sizeof(buf), stdin);

  sleep(5);

  t.safestop();
  
  return 0;
}

