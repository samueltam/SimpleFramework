#ifndef __SIMPLE_APPLICATION__
#define __SIMPLE_APPLICATION__

#include <stdio.h>
#include <signal.h>

#include <vector>
#include <map>

#include "controlled_module_ex.h"

namespace SimpleFramework
{

#define BM_USER_MESSAGE     10000

  class simple_message
  {
    public:
      simple_message(unsigned int code, unsigned int dest, unsigned int src) : 
        code_(code),
        dest_(dest),
        src_(src)
    {
    }

      unsigned long get_code()
      {
        return code_;
      }

      unsigned long get_dest()
      {
        return dest_;
      }

      unsigned long get_src()
      {
        return src_;
      }

    private:
      unsigned int code_;
      unsigned int dest_;
      unsigned int src_;
  };

#define SIMPLE_COMPONENT_INVALID_CODE       0

  class simple_component;

  typedef void (*simple_message_handler) (simple_message& msg);
  typedef struct 
  {
    unsigned int code_;
    simple_message_handler handler_;
  } simple_func_table;

  class simple_component
  {
    public:
      simple_component(unsigned int id, simple_func_table* func_table) : 
        id_(id),
        func_table_(func_table)
    {}
      ~simple_component() { func_table_ = 0; }

      unsigned long id()
      {
        return id_;
      }

      void handle_message(simple_message& msg)
      {
        simple_func_table* ptr = func_table_;

        while (ptr->code_ != SIMPLE_COMPONENT_INVALID_CODE)
        {
          if (ptr->code_ == msg.get_code())
          {
            (*(ptr->handler_))(msg);
            break;
          }

          ptr++;
        }
      }

      void set_message_handler(simple_func_table* func_tab)
      {
        func_table_ = func_tab;
      }

    private:
      unsigned int id_;
      simple_func_table* func_table_;
  };

  class simple_engine : public controlled_module_ex
  {
    public:
      simple_engine(const std::string& name) : SimpleFramework::controlled_module_ex(name) {}
      ~simple_engine() {}

      void add_component(simple_component* comp)
      {
        components_.insert(std::pair<unsigned int, simple_component*>(comp->id(), comp));
      }

    protected:
      virtual void message(const SimpleFramework::_command& cmd)
      {
        controlled_module_ex::message(cmd);

        if (cmd.nCmd == BM_USER_MESSAGE)
        {
          printf("Receive user message!\n");
          simple_message msg = boost::any_cast<simple_message>(cmd.anyParam);
          std::map<unsigned int, simple_component*>::iterator it = components_.find(msg.get_dest());
          if (it != components_.end())
          {
            it->second->handle_message(msg);
          }
        }
      }

    private:
      std::map<unsigned int, simple_component*> components_;
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
      std::vector<simple_engine*> engines_;
      std::map<int, signal_handler> signal_handlers_;
  };

  simple_application* simple_application::instance__(0);

  inline simple_engine* get_engine(const std::string& name)
  {
    simple_application::instance()->get_engine(name);
  }

  class simple_application_exception_base : public std::exception
  {
  };
}

#endif

