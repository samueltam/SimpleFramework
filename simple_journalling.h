#ifndef __SIMPLE_JOURNALLING__
#define __SIMPLE_JOURNALLING__

#include "simple_message.h"
#include "simple_application.h"

namepsace SimpleFramework
{
  typedef enum
  {
    SIMPLE_JOURNALLING_INIT,
    SIMPLE_JOURNALLING_ADD,
    SIMPLE_JOURNALLING_UPDATE,
    SIMPLE_JOURNALLING_DELETE,
    SIMPLE_JOURNALLING_ACTION_MAX
  } simple_journalling_action;

  class simple_journalling_message
  {
    public:
      struct __attribute__((packed)) msg_head
      {
        uint8_t action_;
        uint16_t no_of_objs_;
        uint8_t padding_;
      };

      simple_journalling_message(bufferPtr buffer)
      {
        buffer_ = buffer;
        head_ = (msg_head*) buffer_->buf_;
        data_ = buffer->buf_ + sizeof(msg_head);
      }

      simple_journalling_message(uint8_t action, uint16_t no_of_objs, bufferPtr buffer)
      {
        bufferPtr msg_head_ptr = bufferPtr(new simple_buffer(sizeof(msg_head)));
        msg_head_ptr->action_ = action;
        msg_head_ptr->no_of_objs_ = no_of_objs;

        buffer_ = bufferPtr(new simple_buffer(0));
        *buffer_ = *msg_head_ptr + *buffer;
      }

      ~simple_journalling_message() {}

      simple_journalling_action get_action()
      {
        return msg_head_->action_;
      }

      const bufferPtr get_buffer()
      {
        return buffer_;
      }

      const msg_head& get_head()
      {
        return *head_;
      }

      const char* get_data()
      {
        return data_;
      }

    private:
      bufferPtr buffer_;
      msg_head* head_;
      char* data_;
  };

  class simple_journalling_object
  {
    public:
      simple_journalling_object(uint32_t id) : id_(id) {}
      ~simple_journalling_object() {}

      uint32_t id() { return id_; }

      virtual simple_buffer& operator<<(simple_buffer& msg) = 0;
      virtual simple_buffer& operator>>(simple_buffer& msg) = 0;

    private:
      uint32_t id_;
  };

  typedef boost::shared_ptr<simple_journalling_object> jObjectPtr;

  class simple_journalling_data_set
  {
    public:
      simple_journalling_data_set(uint32_t id) : id_(id) {}
      ~simple_journalling_data_set() {}

      void add_object(jobjectPtr obj)
      {
        objects_.insert(std::pair<uint32_t, jobjectPtr>(obj->id(), obj));
      }

      void remove_object(uint32_t id)
      {
        std::map<uint32_t, jobjectPtr>::iterator it = objects_.find(id);
        if (it != objects_.end())
          objects_.erase(it);
      }

      void handle_journalling_message(simple_journalling_message& msg)
      {
        swtich (msg.get_action())
        {
          case SIMPLE_JOURNALLING_INIT:
            client_addr_ = msg.get_sender();
            local_addr_ = msg.get_receiver();
            check_pointing();
            break;
          case SIMPLE_JOURNALLING_ADD:
            break;
          case SIMPLE_JOURNALLING_UPDATE:
            break;
          case SIMPLE_JOURNALLING_DELETE:
            break;
          default:
        }
      }

      void check_pointing()
      {
        std::map<uint32_t, jObjectPtr>::iterator it;
        for (it = objects_.begin(); it != objects_.end(); it++)
        {
          simple_buffer buf(MAX_SIMPLE_MESSAGE_LEN);
          buf << *(it->second);
          simple_journalling_message j_msg(SIMPLE_JOURNALLING_ADD, 1, buf);
          simple_message msg(local_addr_, client_addr_, SIMPLE_JOURNALLING, j_msg.get_buffer()->buf_, j_msg.get_buffer()->len_);
          msg.send(get_socket());
        }
      }

      void set_server(simple_address& server)
      {
        server_addr_ = server;
      }

      void send_init()
      {
        simple_buffer buf(0);
        simple_journalling_message j_msg(SIMPLE_JOURNALLING_INIT, 0, buf);
        simple_message msg(local_addr_, client_addr_, SIMPLE_JOURNALLING, j_msg.get_buffer()->buf_, j_msg.get_buffer()->len_);
        msg.send(get_socket());
      }

    private:
      uint32_t id_;
      std::map<uint32_t, jObjectPtr> objects_;

      simple_address client_addr_;
      simple_address server_addr_;
      simple_address local_addr_;
  };

}

#endif

