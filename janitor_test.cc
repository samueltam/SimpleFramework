#include <iostream>
#include <string>

#include "janitor.h"

using namespace std;

class class1
{
  public:
    class1() {}
    
    ~class1() {}
  
  public:
    void test()
    {
      SimpleFramework::ObjJanitor ja(*this,&class1::testJanitor);
    }
    
    void testJanitor()
    {
      cout << "hello world" << endl;
    }
};

int main(int argc, char* argv[])
{
  class1 c;
  c.test();
  return 0;
}

