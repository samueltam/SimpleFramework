#ifndef __SIMPLE_APPLICATION__
#define __SIMPLE_APPLICATION__

#include <stdio.h>
#include <signal.h>

#include <vector>
#include <map>

#include "controlled_module_ex.h"

namespace SimpleFramework
{

#define BM_USER_INCOMING_MESSAGE      10000
#define BM_USER_OUTGOING_MESSAGE      BM_USER_INCOMING_MESSAGE+1

#define SIMPLE_FUNCTION_RESERV   1000

  class simple_engine;

  void register_engine(uint32_t func_start, uint32_t func_end, simple_engine* engine);

  typedef void (*simple_message_handler) (simple_message& msg);
  typedef struct 
  {
    unsigned int code_;
    simple_message_handler handler_;
  } simple_function_table;

  class simple_engine : public controlled_module_ex
  {
    public:
      simple_engine(const std::string& name, simple_function_table* func_table) : 
        SimpleFramework::controlled_module_ex(name), 
        func_table_(func_table) 
    {
      uint32_t func_start = func_table_[0].code_;
      uint32_t func_end;
      simple_function_table* ptr = func_table_;

      while (ptr->handler_ != 0)
        ptr++;

      func_end = ptr->code_;

      register_engine(func_start, func_end, this);
    }

      ~simple_engine() {}

    protected:
      virtual void message(const SimpleFramework::_command& cmd)
      {
        controlled_module_ex::message(cmd);

        if (func_table_ == 0)
          return;

        simple_function_table* ptr = func_table_;

        if (cmd.nCmd != BM_USER_INCOMING_MESSAGE)
          return;

        simple_message msg = boost::any_cast<simple_message>(cmd.anyParam);

        while (ptr->handler_ != 0)
        {
          if (msg.get_data()->head_->func_no_ == ptr->code_)
          {
            printf("Receive user message!\n");
            (*ptr->handler_)(msg);
            break;
          }

          ptr++;
        }
      }

    private:
      simple_function_table* func_table_;
  };

  class simple_dispatcher
  {
    public:
      typedef struct
      {
        uint32_t func_start_;
        uint32_t func_end_;
        simple_engine* engine_;
      } dispatch_info;

      static simple_dispatcher* instance()
      {
        if (instance__ == 0)
          instance__ = new simple_dispatcher();

        return instance__;
      }

      void register_engine(uint32_t func_start, uint32_t func_end, simple_engine* engine)
      {
        dispatch_info info;

        info.func_start_ = func_start;
        info.func_end_ = func_end;
        info.engine_ = engine;

        dispatch_info_list_.push_back(info);
      }

      void dispatch(uint32_t func_no, simple_message& msg)
      {
        for (std::vector<dispatch_info>::iterator it = dispatch_info_list_.begin();
            it != dispatch_info_list_.end();
            it++)
        {
          if (func_no >= it->func_start_ && func_no <= it->func_end_)
          {
            it->engine_->postmessage(BM_USER_INCOMING_MESSAGE, msg);
            return;
          }
        }

        printf("Can't handle message with func_no %d\n", func_no);
      }

    private:
      simple_dispatcher() {}

      static simple_dispatcher* instance__;

      std::vector<dispatch_info> dispatch_info_list_;
  };

  simple_dispatcher* simple_dispatcher::instance__(0);

  inline void register_engine(uint32_t func_start, uint32_t func_end, simple_engine* engine)
  {
    simple_dispatcher::instance()->register_engine(func_start, func_end, engine);
  }

  inline void dispatch(uint32_t func_no, simple_message& msg)
  {
    simple_dispatcher::instance()->dispatch(func_no, msg);
  }

  class simple_socket_server : public controlled_module
  {
    public:
      simple_socket_server(boost::shared_ptr<simple_udp_socket> socket)
      {
        socket_ = socket;
      }

      virtual bool work()
      {
        simple_address sender;
        char buffer[MAX_MESSAGE_LEN];
        simple_message::msg_header* msg_hdr = (simple_message::msg_header*) buffer;
        int recv_len;

        if (socket_->select(10000))
        {
          recv_len = socket_->recv_msg(sender, (void*) buffer, MAX_MESSAGE_LEN);

          simple_message msg(sender, socket_->get_address(), msg_hdr->func_no_, buffer + sizeof(simple_message::msg_header), recv_len - sizeof(simple_message::msg_header));
          dispatch(msg_hdr->func_no_, msg);
        }

        return true;
      }

    private:
      boost::shared_ptr<simple_udp_socket> socket_;
  };

  class simple_application
  {
    public:
      typedef void (*signal_handler)(int);

      static simple_application* instance()
      {
        if (instance__ == 0)
          instance__ = new simple_application();

        return instance__;
      }

      static void exit_signal_handler(int signo)
      {
        printf("receive signal %d\n", signo);
        instance()->stop();
      }

      void initialize(int argc, char* argv[])
      {
        argc_ = argc;
        argv_ = argv;

        signal_handlers_[SIGTERM] = &simple_application::exit_signal_handler;
        signal_handlers_[SIGINT] = &simple_application::exit_signal_handler;
        signal_handlers_[SIGHUP] = &simple_application::exit_signal_handler;
        signal_handlers_[SIGILL] = &simple_application::exit_signal_handler;
        signal_handlers_[SIGABRT] = &simple_application::exit_signal_handler;
        signal_handlers_[SIGKILL] = &simple_application::exit_signal_handler;
        signal_handlers_[SIGSTOP] = &simple_application::exit_signal_handler;

        socket_ = boost::shared_ptr<simple_udp_socket>(new simple_udp_socket("localhost", 10000));
        socket_server_ = boost::shared_ptr<simple_socket_server>(new simple_socket_server(socket_)); 
      }

      ~simple_application() {}
      virtual void usage() { printf("Print your usage here!"); }

      void start()
      {
        int ret;
        sigset_t sigs;

        sigemptyset(&sigs);
        sigaddset(&sigs, SIGTERM);
        sigaddset(&sigs, SIGINT);
        sigaddset(&sigs, SIGHUP);
        sigaddset(&sigs, SIGILL);
        sigaddset(&sigs, SIGABRT);
        sigaddset(&sigs, SIGKILL);
        sigaddset(&sigs, SIGUSR1);
        sigaddset(&sigs, SIGUSR2);
        sigaddset(&sigs, SIGSTOP);
        sigaddset(&sigs, SIGALRM);

        ret = pthread_sigmask(SIG_BLOCK, &sigs, NULL);

        for (std::vector<simple_engine*>::iterator it = engines_.begin(); it != engines_.end(); it++)
        {
          (*it)->safestart();
          printf("engine %s started\n", (*it)->name().c_str());
        }

        printf("all engines started\n");

        socket_server_->start();

        printf("socket server started\n");

        while (!stop_)
        {
          int signo;

          ret = sigwait(&sigs, &signo);

          if (ret != 0)
          {
            stop();
          }

          printf("receive signal %d\n", signo);

          std::map<int, signal_handler>::iterator it = signal_handlers_.find(signo);

          if (it != signal_handlers_.end())
          {
            signal_handler handler = it->second;
            handler(signo);
          }
        }

        printf("stopping socket server\n");
        socket_server_->die();

        printf("stopping all engines\n");
        for (std::vector<simple_engine*>::iterator it = engines_.begin(); it != engines_.end(); it++)
        {
          (*it)->safestop();
        }

        exit(0);

      }

      void stop()
      {
        stop_ = true;
      }

      void add_engine(simple_engine* engine)
      {
        assert(engine != 0);

        if (get_engine(engine->name()) == 0)
          engines_.push_back(engine);
        else
          printf("The engine with name \"%s\" already exist!", engine->name().c_str());
      }

      simple_engine* get_engine(const std::string& name)
      {
        for (std::vector<simple_engine*>::iterator it = engines_.begin(); it != engines_.end(); it++)
        {
          if ((*it)->name() == name)
            return *it;
        }

        return 0;
      }

      const simple_udp_socket& get_socket() { return *socket_; }

      void install_signal_handler(int signo, signal_handler handler)
      {
        signal_handlers_[signo] = handler;
      }

    private:
      simple_application() : 
        argc_(0),
        argv_(0),
        stop_(false)
    {}

      static simple_application* instance__;

      int argc_;
      char** argv_;
      bool stop_;
      boost::shared_ptr<simple_socket_server> socket_server_;
      boost::shared_ptr<simple_udp_socket> socket_;

      std::vector<simple_engine*> engines_;
      std::map<int, signal_handler> signal_handlers_;
  };

  simple_application* simple_application::instance__(0);

  inline simple_engine* get_engine(const std::string& name)
  {
    return simple_application::instance()->get_engine(name);
  }

  inline const simple_udp_socket& get_socket()
  {
    return simple_application::instance()->get_socket();
  }

  class simple_application_exception_base : public std::exception
  {
  };
}

#endif

