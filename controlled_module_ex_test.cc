#include <stdio.h>
#include <iostream>

#include "controlled_module_ex.h"

using namespace std;

class thdex1 : public SimpleFramework::controlled_module_ex
{
  protected:
    virtual void message(const SimpleFramework::_command & cmd)
    {
      controlled_module_ex::message(cmd);
      if (cmd.nCmd == BM_USER+1)
      {
        cout << "get message" << endl;
      }
    }
};

struct mystruct2
{
  string a;
  int b;
};

class thdex2 : public SimpleFramework::controlled_module_ex
{
  protected:
    virtual void message(const SimpleFramework::_command & cmd)
    {
      controlled_module_ex::message(cmd);
      if(cmd.nCmd==BM_USER+1)
      {
        cout << "get integer:" << boost::any_cast<int>(cmd.anyParam) << endl;
      }
      if(cmd.nCmd==BM_USER+2)
      {
        cout << "get string:" << boost::any_cast<string>(cmd.anyParam) << endl;
      }
      if(cmd.nCmd==BM_USER+3)
      {
        mystruct2 par = boost::any_cast<mystruct2>(cmd.anyParam);
        cout << "get mystruct:" << par.a << "," << par.b << endl;
      }
    }
};

struct mystruct3
{
  boost::shared_ptr<char> data;
  int datalen;
};

class thdex3 : public SimpleFramework::controlled_module_ex
{
  protected:
    virtual void message(const SimpleFramework::_command & cmd)
    {
      controlled_module_ex::message(cmd);
      if(cmd.nCmd==BM_USER+1)
      {
        cout << "get sharedptr" << endl; //仅仅得到数据，得不到数据长度
      }
      if(cmd.nCmd==BM_USER+2)
      {
        mystruct3 par = boost::any_cast<mystruct3>(cmd.anyParam);
        cout << "get sharedptr len:" << par.datalen << endl;
      }
    }
};

class thdex4 : public SimpleFramework::controlled_module_ex
{
  protected:
    boost::any on_command(const unsigned int command,const boost::any par)
    {
      if(command==1)
      {
        cout << "on command" << endl;
        return 0;
      }
      if(command==2)
      {
        cout << "on command,par:" << boost::any_cast<string>(par) << endl;
        return 0;
      }
      if(command==3)
      {
        return true;
      }
      else
        return controlled_module_ex::on_command(command,par);
    }
};

class thdex5: public SimpleFramework::controlled_module_ex
{
  protected:
    virtual void on_timer(const SimpleFramework::controlled_timer *p)
    {
      cout << "ontimer" << endl;
    }
};

int main(int argc, char* argv[])
{
  thdex1 t1;

  t1.safestart();
  t1.postmessage(BM_USER+1);
  printf("Please hit Enter:");
  char buf[10];
  fgets(buf, sizeof(buf), stdin);
  t1.safestop();

  thdex2 t2;
  t2.safestart();
  t2.postmessage(BM_USER+1,123);
  t2.postmessage(BM_USER+2,string("hello world"));
  mystruct2 par;
  par.a = "hello world";
  par.b = 123;
  t2.postmessage(BM_USER+3,par);
  printf("Please hit Enter:");
  fgets(buf, sizeof(buf), stdin);
  t2.safestop();

  thdex3 t3;
  t3.safestart();
  t3.postmessage(BM_USER+1,boost::shared_ptr<char>(new char[1000]));
  mystruct3 par3;
  par3.datalen = 1000;
  par3.data = boost::shared_ptr<char>(new char[par3.datalen]);
  t3.postmessage(BM_USER+2,par3);
  printf("Please hit Enter:");
  fgets(buf, sizeof(buf), stdin);
  t3.safestop();

  thdex4 t4;
  t4.safestart();
  t4.execute(1,0);//等待子线程处理完成
  t4.execute(2,string("hello world"));//带参数 等待子线程完成
  bool rs = boost::any_cast<bool>(t4.execute(3,0));//等待子线程处理完成，并取得返回值
  cout << "get thread result:" << rs << endl;
  boost::any timeout = t4.execute(4,0,1000);//等待子线程处理，超时1秒
  if(timeout.empty())
    cout << "timeout " << endl;
  printf("Please hit Enter:");
  fgets(buf, sizeof(buf), stdin);
  t4.safestop();

  thdex5 t5;
  SimpleFramework::controlled_timer timer;
  t5.safestart();
  timer.starttimer(3,&t5);
  printf("Please hit Enter:");
  fgets(buf, sizeof(buf), stdin);
  timer.stoptimer();
  t5.safestop();
  
  return 0;
}

