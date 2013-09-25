#ifndef __SIMPLE_SOCKET_H__
#define __SIMPLE_SOCKET_H__

#include <sys/resource.h>
#include <sys/types.h>    /* basic system data types */
#include <sys/socket.h>   /* basic socket definitions */
#include <sys/time.h>     /* timeval{} for select() */
#include <time.h>         /* timespec{} for pselect() */
#include <netinet/in.h>   /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>    /* inet(3) functions */
#include <errno.h>
#include <fcntl.h>        /* for nonblocking */
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>     /* for S_xxx file mode constants */
#include <sys/uio.h>      /* for iovec{} and readv/writev */
#include <unistd.h>
#include <sys/wait.h>
#include <sys/un.h>       /* for Unix domain sockets */
#include <sys/select.h>   /* for convenience */
#include <inttypes.h>     /* for integer types */

#include <string>
#include <sstream>
#include <exception>

#include <boost/shared_ptr.hpp>

namespace SimpleFramework
{
#define SIMPLE_SOCKET_EXCEPTION_MSG_LEN 1024

  class simple_socket_exception_base : public std::exception
  {
    public:
      ~simple_socket_exception_base() throw() {}
    protected:
      std::string err_msg;
  };

  class simple_socket_exception_socket_operation : public simple_socket_exception_base
  {
    public:
      simple_socket_exception_socket_operation(std::string operation, int err)
      {
        std::stringstream ss;

        ss << "Exception in socket operation. " 
          << "Operation : " << operation 
          << ", return code : %d." << err;

        err_msg = ss.str();
      }

      ~simple_socket_exception_socket_operation() throw() {}

      const char* what() const throw()
      {
        return err_msg.c_str();
      }
  };

  struct simple_address
  {
    sockaddr_in addr;

    simple_address(int family = AF_INET)
    {
      memset(&addr, 0, sizeof(sockaddr_in));
      addr.sin_family = family;
    }

    ~simple_address()
    {
    }

    simple_address(const char * address, int port)
    {
      if (address)
      {
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr((char*) address);
      }
    }

    simple_address(const sockaddr_in & saddr)
    {
      memmove(&addr, &saddr, sizeof(sockaddr_in));
      addr.sin_family = AF_INET;
    }

    simple_address(const simple_address& address)
    {
      memmove(&addr, &address.addr, sizeof(sockaddr_in));
    }

    simple_address& operator = (const simple_address& address)
    {
      memmove(&addr, &address.addr, sizeof(sockaddr_in));
      return *this;
    }

    bool operator == (const simple_address& address) const
    {
      if (addr.sin_addr.s_addr == address.addr.sin_addr.s_addr &&
          addr.sin_port == address.addr.sin_port)
        return true;

      return false;
    }

    bool operator != (const simple_address& address) const
    {
      if (addr.sin_addr.s_addr == address.addr.sin_addr.s_addr &&
          addr.sin_port == address.addr.sin_port)
        return false;

      return true;
    }

  };

#define MAX_MESSAGE_LEN     65507

  class simple_udp_socket
  {
    public:
      simple_udp_socket() :
        socket_id_(-1) {}

      simple_udp_socket(const char* addr, int port, bool block = false) :
        local_addr_(addr, port),
        block_(block)
    {
      open();
    }

      simple_udp_socket(const simple_udp_socket& socket)
      {
        *this = socket;
      }
      virtual ~simple_udp_socket()
      {
        close();
      }

      simple_udp_socket& operator=(const simple_udp_socket& socket)
      {
        socket_id_ = socket.socket_id_;
        local_addr_ = socket.local_addr_;
        return *this;
      }

      bool operator==(const simple_udp_socket& socket)
      {
        return socket_id_ == socket.socket_id_ ? true : false;
      }

      int recv_msg(simple_address& sender, void* data, int maxsize)
      {
        if (!is_valid())
          return 0;

        socklen_t size = sizeof(sockaddr);
        int len = recvfrom(socket_id_, (char*)data, maxsize, 0, (struct sockaddr*) &(sender.addr), &size);
        sender.addr.sin_port = ntohs(sender.addr.sin_port);
        printf("receive %d bytes\n", len);

        return len;
      }

      int send_msg(simple_address& receiver, const void* data, int size)
      {
        if (size < 0)
          return -1;

        if (!is_valid())
          return -1;

        if (size <= 0)
          return 0;

        printf("send to port %d\n", ntohs(receiver.addr.sin_port));
        int len = sendto(socket_id_, (char*)data, size, 0, (struct sockaddr*) &(receiver.addr), sizeof(sockaddr_in));
        printf("send %d bytes\n", len);

        return len;
      }

      bool is_valid() const { return socket_id_ != -1 ? true : false; }
      int get_socket() const { return socket_id_; }
      simple_address get_address() const { return local_addr_; }

      void open()
      {
        socket_id_ = ::socket(AF_INET, SOCK_DGRAM, 0);

        if (socket_id_ == -1)
          throw simple_socket_exception_socket_operation("socket()", socket_id_);

        int recv_buf = 20*1024;
        int send_buf = 10*1024;
        int ret;

        ret = setsockopt(socket_id_, SOL_SOCKET, SO_RCVBUF, (char*) &recv_buf, sizeof(recv_buf));
        ret = setsockopt(socket_id_, SOL_SOCKET, SO_SNDBUF, (char*) &send_buf, sizeof(send_buf));

        int opt = 1;
        int nb = 0;

        nb = setsockopt(socket_id_, SOL_SOCKET, SO_BROADCAST, (char*) &opt, sizeof(opt));

        if (!block_)
        {
          int flags = fcntl(socket_id_, F_GETFL);

          if (flags < 0)
            throw simple_socket_exception_socket_operation("fcntl()", flags);

          flags |= O_NONBLOCK;

          if ((ret = fcntl(socket_id_, F_SETFL, flags)) < 0)
            throw simple_socket_exception_socket_operation("fcntl()", ret);
        }

        sockaddr_in address;
        memset(&address, 0, sizeof(sockaddr_in));
        address.sin_family = AF_INET;
        address.sin_port = local_addr_.addr.sin_port;
        address.sin_addr.s_addr = INADDR_ANY;

        if ((ret = bind(socket_id_, (struct sockaddr*) &address, sizeof(sockaddr))) < 0)
          throw simple_socket_exception_socket_operation("bind()", ret);
      }

      void close()
      {
        if (socket_id_ > 0)
        {
          ::close(socket_id_);
          socket_id_ = -1;
        }
      }

      bool select(int usec)
      {
        if (!is_valid())
        {
          printf("socket invalid!\n");
          return false;
        }

        fd_set readfds;
        struct timeval tv;
        int ret;

        FD_ZERO(&readfds);
        FD_SET(socket_id_, &readfds);

        tv.tv_sec = 0;
        tv.tv_usec = usec;

        ret = ::select(socket_id_ + 1, &readfds, NULL, NULL, &tv);

        if (ret == -1)
        {
          printf("select error\n");

          if (errno == EINTR)
            return false;
          else
            throw simple_socket_exception_socket_operation("select()", errno);
        }

        if (ret == 0)
          return false;

        return true;
      }

    private:
      int socket_id_;
      simple_address local_addr_;
      bool block_;
  };

  class simple_message
  {
    public:
      struct __attribute__((packed)) msg_header
      {
        uint32_t msg_len_;
        uint32_t func_no_;
        uint32_t padding_;
      };

      struct msg_data
      {
        char* buf_;
        msg_header* head_;
        char* data_;
        int data_len_;
        int total_len_;

        msg_data(const char* data, int len)
        {
          total_len_  = len + sizeof(msg_header);
          buf_ = new char[total_len_];
          memset(buf_, 0, total_len_);
          memcpy(buf_ + sizeof(msg_header), data, len);

          head_ = (msg_header*) buf_;
          data_ = buf_ + sizeof(msg_header);
          data_len_ = len;
        }

        ~msg_data()
        {
          if (buf_ != 0)
            delete buf_;
        }
        
        msg_data& operator=(const msg_data& data)
        {
          buf_ = data.buf_;
          head_ = data.head_;
          data_ = data.data_;
          data_len_ = data.data_len_;
          total_len_ = data.total_len_;
        }
      };

      simple_message() :
        sender_(AF_INET),
        receiver_(AF_INET)
        {}

      simple_message(simple_address sender, simple_address receiver, uint32_t func_no, const char* data, int len)
      {
        sender_ = sender;
        receiver_ = receiver;
        data_ = boost::shared_ptr<msg_data>(new msg_data(data, len));
        data_->head_->msg_len_ = len;
        data_->head_->func_no_ = func_no;
      }

      ~simple_message() {}

      simple_message& operator=(const simple_message& msg)
      {
        sender_ = msg.sender_;
        receiver_ = msg.receiver_;
        data_ = msg.data_;
      }

      void send(simple_udp_socket& socket)
      {
        socket.send_msg(receiver_, data_->buf_, data_->total_len_);
      }

      boost::shared_ptr<msg_data> get_data()
      {
        return data_;
      }

    private:
      simple_address sender_;
      simple_address receiver_;
      boost::shared_ptr<msg_data> data_;
  };
}

#endif 

