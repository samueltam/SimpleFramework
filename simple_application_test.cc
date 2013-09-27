#include <iostream>
#include <string>

#include "simple_message.h"
#include "simple_application.h"

using namespace std;

class test_component
{
  public:
    static void handle_test_message(SimpleFramework::simple_message& msg)
    {
      cout << "receive test message" << endl;

      const char* ptr = msg.get_data();
      printf("receive: ");
      for (int i = 0; i < msg.get_head().msg_len_; i++)
      {
        printf("0x%x ", ptr[i]);
      }
      printf("\n");

      SimpleFramework::simple_message msg_resp(msg.get_receiver(), msg.get_sender(), 0, ptr, msg.get_head().msg_len_);
      msg_resp.send(SimpleFramework::get_socket());
    }
};

SimpleFramework::simple_function_table test_functions[] =
{
  {SIMPLE_FUNCTION_RESERV, &test_component::handle_test_message},
  {SIMPLE_FUNCTION_RESERV+1, 0}
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
      SimpleFramework::simple_message msg1(sender, receiver, SIMPLE_FUNCTION_RESERV, text, 8);

      msg1.send(*socket_);

      char buffer[MAX_SIMPLE_MESSAGE_LEN];
      int len;
      len = socket_->recv_msg(receiver, (void*) buffer, MAX_SIMPLE_MESSAGE_LEN);

      printf("receive %d bytes from server!\n", len);

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
  timer.starttimer(1,&t5);

  app->start();
}

