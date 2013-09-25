#include <iostream>
#include <string>

#include "simple_socket.h"
#include "simple_application.h"

using namespace std;

class test_component
{
  public:
    static void handle_test_message(SimpleFramework::simple_message& msg)
    {
      cout << "receive test message" << endl;

      boost::shared_ptr<SimpleFramework::simple_message::msg_data> data = msg.get_data();

      char* ptr = data->data_;
      printf("receive: ");
      for (int i = 0; i < data->data_len_; i++)
      {
        printf("0x%x ", ptr[i]);
      }
      printf("\n");
    }
};

SimpleFramework::simple_function_table test_functions[] =
{
  {BM_USER_MESSAGE, &test_component::handle_test_message},
  {BM_USER_MESSAGE+1, 0}
};

class thdex5: public SimpleFramework::controlled_module_ex
{
  public:
    thdex5() : SimpleFramework::controlled_module_ex("thdex5") 
    {
      socket_ = boost::shared_ptr<SimpleFramework::simple_udp_socket>(new SimpleFramework::simple_udp_socket("localhost", 10001));
    }

  protected:
    virtual void on_timer(const SimpleFramework::controlled_timer *p)
    {
      cout << "ontimer" << endl;

      SimpleFramework::simple_address sender("localhost", 10001);
      SimpleFramework::simple_address receiver("localhost", 10000);
      char* text = "12345678";
      SimpleFramework::simple_message msg(sender, receiver, BM_USER_MESSAGE, text, 8);

      msg.send(*socket_);

      //socket_->send_msg(receiver, text, 8);
    }

  private:
    boost::shared_ptr<SimpleFramework::simple_udp_socket> socket_;
};

int main(int argc, char* argv[])
{
  SimpleFramework::simple_application* app = SimpleFramework::simple_application::instance();

  app->initialize(argc, argv);

  SimpleFramework::simple_engine* engine1 = new SimpleFramework::simple_engine("test_compnont", test_functions);
  app->add_engine(engine1);

  thdex5 t5;
  SimpleFramework::controlled_timer timer("test_timer");
  t5.safestart();
  timer.starttimer(3,&t5);

  app->start();
}

