#include <iostream>
#include <string>

#include "simple_application.h"

using namespace std;

class comp1 : public SimpleFramework::simple_component
{
  public:
    comp1(unsigned int id, SimpleFramework::simple_func_table* func_tab) : SimpleFramework::simple_component(id, func_tab) {}
    static void handle_test_message(SimpleFramework::simple_message& msg)
    {
      printf("get test message!\n");
    }
};

SimpleFramework::simple_func_table func_tab1[] =
{
  {1, &comp1::handle_test_message},
  {SIMPLE_COMPONENT_INVALID_CODE, 0}
};

class engine1 : public SimpleFramework::simple_engine
{
  public:
    engine1() : SimpleFramework::simple_engine("engine1") {}

};

class engine2: public SimpleFramework::simple_engine
{
  public:
    engine2() : SimpleFramework::simple_engine("engine2") {}

  protected:
    virtual void on_timer(const SimpleFramework::controlled_timer *p)
    {
      SimpleFramework::simple_message msg(1, 1, 1);
      SimpleFramework::get_engine("engine1")->postmessage(BM_USER_MESSAGE, msg);
      cout << "ontimer" << endl;
    }
};

int main(int argc, char* argv[])
{
  SimpleFramework::simple_application* app = SimpleFramework::simple_application::instance();

  app->initialize(argc, argv);

  engine1* e1 = new engine1();
  comp1* c1 = new comp1(1, func_tab1);
  e1->add_component(c1);
  app->add_engine(e1);

  SimpleFramework::controlled_timer timer("test_timer");
  engine2* e2 = new engine2();
  timer.starttimer(3, e2);
  app->add_engine(e2);

  app->start();
}

