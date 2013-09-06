#ifndef __CONTROLLED_MODULE__
#define __CONTROLLED_MODULE__

#include <boost/utility.hpp>   
#include <boost/thread/condition.hpp>   
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

#include "janitor.h"

class controlled_module_implement : boost::noncopyable 
{ 
  public:
    controlled_module_implement()
      : active_(false),
      command_exit_(false)
    {
    }   
    
    boost::condition module_is_exit;
    boost::mutex monitor;
    
    void active(bool ac)
    {
      boost::mutex::scoped_lock lk(monitor);

      if (!(active_ = ac)) 
        module_is_exit.notify_all();
      else
        command_exit_=false;
    }

    bool command_exit()
    {
      boost::mutex::scoped_lock lk(monitor);
      return   command_exit_;
    }
    
    bool active_, command_exit_;   
};

class controlled_module : boost::noncopyable
{
  public:
    virtual void run()
    {
      ObjJanitor janitor(*impl_, &controlled_module_implement::active, false); 

      impl_->active(true);
      
      {
        ObjJanitor janitor(*this, &controlled_module::release);
        
        if (this->initialize())
        {
          m_live = true;
          set_event_init(true);

          while (!impl_->command_exit() && this->islive() && this->work())
          {
          }
        }
        else
        {
          m_live = false;
          set_event_init(true);
        }
      }  
    }
    
    bool exit(unsigned long sec=0)
    {
      boost::mutex::scoped_lock lk(impl_->monitor);

      impl_->command_exit_ = true;

      while (impl_->active_)
      {
        if (sec)
        {
          boost::xtime xt;
          boost::xtime_get(&xt, boost::TIME_UTC);
          xt.sec += sec; 
          if (!impl_->module_is_exit.timed_wait(lk,xt))
            return false;
        }
        else
          impl_->module_is_exit.wait(lk);
      }
      
      return   true;
    }
  
  protected:
    controlled_module() :
      impl_(new controlled_module_implement),
      m_live(false),
      m_initialized(false),
      m_sleeptime(10)
    {}

    virtual ~controlled_module()
    {
      if (m_live)
        stop();
      
      delete   impl_;
    }
  
  private:
    virtual bool initialize()
    {
      return true;
    }
    
    virtual void release()
    {
    }

    inline void set_event_init(bool initialized)
    {
      {
        boost::mutex::scoped_lock lock(m_event_init_mutex);  
        m_initialized = initialized;
      }
      m_event_init.notify_all();
    }
  
  protected:
    virtual bool work()
    {
      sleep(this->m_sleeptime);
      return true;
    }
    
    int m_sleeptime;
  
  private:
    bool m_live;
    boost::condition m_event_init;
    boost::mutex m_event_init_mutex;
    mutable bool m_initialized;
    controlled_module_implement* impl_;
  
  public:
    bool start()
    {
      boost::thread thd(boost::bind(&controlled_module::run, this));

      boost::mutex::scoped_lock lock(m_event_init_mutex);  
      while (!m_initialized)
      {
        m_event_init.wait(lock);
      }

      return m_live;
    }
    
    void stop()
    {
      m_live = false;
      exit(0);
    }
    
    bool islive()
    {
      return m_live;
    }
    
    void die()
    {
      m_live = false;
      set_event_init(true);
    }
    
    void setsleeptime(int n)
    {
      m_sleeptime = n;
    }
};

#endif

