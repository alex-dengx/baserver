//
// server_work.hpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2009, 2012 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BASTOOL_SERVER_WORK_HPP
#define BASTOOL_SERVER_WORK_HPP

#include <bas/service_handler.hpp>
#include <bas/client.hpp>
#include <bastool/client_work.hpp>
#include <bastool/client_work_allocator.hpp>
#include <iostream>

namespace bastool {

/// Define I/O state machine code.
#define BAS_STATE_NONE                    0x0000

#define BAS_STATE_DO_READ                 0x0002
#define BAS_STATE_DO_WRITE                0x0004
#define BAS_STATE_DO_CLOSE                0x00EF
#define BAS_STATE_DO_CLIENT_OPEN          0x0100
#define BAS_STATE_DO_CLIENT_READ          0x0200
#define BAS_STATE_DO_CLIENT_WRITE         0x0400
#define BAS_STATE_DO_CLIENT_WRITE_READ    0x0600
#define BAS_STATE_DO_CLIENT_CLOSE         0xEF00

#define BAS_STATE_ON_OPEN                 0x0011
#define BAS_STATE_ON_READ                 0x0012
#define BAS_STATE_ON_WRITE                0x0014
#define BAS_STATE_ON_CLOSE                0x00FF
#define BAS_STATE_ON_CLIENT_OPEN          0x1100
#define BAS_STATE_ON_CLIENT_READ          0x1200
#define BAS_STATE_ON_CLIENT_WRITE         0x1400
#define BAS_STATE_ON_CLIENT_CLOSE         0xFF00

using namespace bas;

/// Define status information of I/O.
struct status_t
{
  /// Define type reference of std::size_t.
  typedef std::size_t size_t;

  /// I/O state machine code.
  size_t state;

  /// Bytes transferred in last I/O.
  size_t bytes_transferred;

  /// Error code in last I/O.
  boost::system::error_code ec;

  /// Target server address.
  boost::asio::ip::tcp::endpoint endpoint;

  /// Default constructor.
  status_t()
  {
    clear();
  }

  /// Set all member to default value.
  void clear()
  {
    state = BAS_STATE_NONE;
    bytes_transferred = 0;
    ec = boost::system::error_code();
    endpoint = boost::asio::ip::tcp::endpoint();
  }

  /// Set special member to given value.
  void set(size_t s,
           size_t b = 0,
           boost::system::error_code e = boost::system::error_code())
  {
    state = s;
    bytes_transferred = b;
    ec = e;
  }
};

/// Simple business global storage for none resource used.
class bgs_none
{
public:
  bgs_none()   {}
  void init()  {}
  void close() {}  
};

/// Demo class for handle echo_server business process.
template<typename Biz_Global_Storage>
class biz_echo
{
public:
  typedef boost::shared_ptr<Biz_Global_Storage> bgs_ptr;

  /// Constructor.
  biz_echo(bgs_ptr bgs)
    : bgs_(bgs)
  {
  }

  /// Main function to process business logical operations.
  void process(status_t& status, io_buffer& input, io_buffer& output)
  {
    switch (status.state)
    {
      case BAS_STATE_ON_OPEN:
        status.state = BAS_STATE_DO_READ;
        break;

      case BAS_STATE_ON_READ:
        status.state = BAS_STATE_DO_WRITE;
        break;

      case BAS_STATE_ON_WRITE:
        status.state = BAS_STATE_DO_READ;
        break;

      case BAS_STATE_ON_CLOSE:
        switch (status.ec.value())
        {
          // Operation successfully completed.
          case 0:
          case boost::asio::error::eof:
            break;

          // Connection breaked.
          case boost::asio::error::connection_aborted:
          case boost::asio::error::connection_reset:
          case boost::asio::error::connection_refused:
            std::cout << "C";
            break;

          // Connection timeout.
          case boost::asio::error::timed_out:
            std::cout << "T";
            break;
        
          case boost::asio::error::no_buffer_space:
          default:
            std::cout << "O";
            break;
        }

        break;

      default:
        status.state = BAS_STATE_DO_CLOSE;
        break;
    }
  }

  /// Business Global storage for holding application resources.
  bgs_ptr bgs_;
};

/// Object for handle server asynchronous operations.
template<typename Biz_Handler>
class server_work
{
public:
  /// Define type reference of std::size_t.
  typedef std::size_t size_t;

  /// Define type reference of handlers and allocators.
  typedef server_work<Biz_Handler> server_work_t;
  typedef client_work<Biz_Handler> client_work_t;
  typedef client_work_allocator<Biz_Handler> client_work_allocator_t;
  typedef service_handler<server_work_t> server_handler_t;
  typedef service_handler<client_work_t> client_handler_t;
  typedef client<client_work_t, client_work_allocator_t> client_t;

  /// Define shared_ptr for holding pointers.
  typedef boost::shared_ptr<client_t> client_ptr;
  typedef boost::shared_ptr<Biz_Handler> biz_ptr;

  /// Constructor.
  server_work(Biz_Handler* biz, client_ptr client)
    : biz_(biz),
      client_(client),
      client_handler_(0),
      status_()
  {
    BOOST_ASSERT(biz != 0);
  }
  
  /// Destructor.
  ~server_work()
  {
    client_.reset();
    biz_.reset();
  }

  /// Quick reference to read_buffer for normally used.
  template<typename Service_Handler>
  bas::io_buffer& io_buffer(Service_Handler& handler)
  {
    return handler.read_buffer();
  }

  /// Execute asynchronous I/O operations.
  void do_io(server_handler_t& handler)
  {
    switch (status_.state)
    {
      case BAS_STATE_DO_READ:
        handler.async_read_some();
        break;

      case BAS_STATE_DO_WRITE:
        handler.async_write(boost::asio::buffer(io_buffer(handler).data(), io_buffer(handler).size()));
        break;

      case BAS_STATE_DO_CLIENT_OPEN:
      case BAS_STATE_DO_CLIENT_CLOSE:
        if (client_.get() != 0)
        {
          if (client_handler_ != 0)
          {
            // Notify child to close.
            client_handler_->parent_post(bas::event(bas::event::close));
            client_handler_ = 0;
          }
        
          if (status_.state == BAS_STATE_DO_CLIENT_OPEN)
            client_->connect(handler, status_.endpoint);
        }

        break;

      case BAS_STATE_DO_CLIENT_READ:
        if (client_handler_ != 0)
        {
          if (io_buffer(*client_handler_).space() == 0)
          {
            // Notify parent for child read error.
            handler.child_post(bas::event(bas::event::read,
                0,
                boost::system::error_code(boost::asio::error::no_buffer_space, boost::system::get_system_category())));
          }
          else
          {
            // Notify child to read.
            io_buffer(*client_handler_).crunch();
            client_handler_->parent_post(bas::event(bas::event::read));
          }
        }

        break;

      case BAS_STATE_DO_CLIENT_WRITE:
      case BAS_STATE_DO_CLIENT_WRITE_READ:
        if (client_handler_ != 0)
        {
          // Clear client buffer here.
          io_buffer(*client_handler_).clear();
          if (io_buffer(*client_handler_).space() < io_buffer(handler).size())
          {
            // Notify parent for child write error.
            handler.child_post(bas::event(bas::event::write,
                0,
                boost::system::error_code(boost::asio::error::no_buffer_space, boost::system::get_system_category())));
          }
          else
          {
            memcpy(io_buffer(*client_handler_).data(), io_buffer(handler).data(), io_buffer(handler).size());
            io_buffer(*client_handler_).produce(io_buffer(handler).size());
            
            if (status_.state == BAS_STATE_DO_CLIENT_WRITE_READ)
            {
              // Notify child to write and read.
              client_handler_->parent_post(bas::event(bas::event::write_read));
            }
            else
              // Notify child to write.
              client_handler_->parent_post(bas::event(bas::event::write));
          }
        }

        break;

      case BAS_STATE_DO_CLOSE:
      default:
        if (client_handler_ != 0)
        {
          client_handler_->parent_post(event(event::close));
          client_handler_ = 0;
        }

        handler.close();

        break;
    }
  }

  void on_set_child(server_handler_t& handler, client_handler_t* client_handler)
  {
    client_handler_ = client_handler;
  }

  void on_clear(server_handler_t& handler)
  {
  }

  void on_open(server_handler_t& handler)
  {
    status_.clear();
    status_.set(BAS_STATE_ON_OPEN);
    io_buffer(handler).clear();
    biz_->process(status_, io_buffer(handler), io_buffer(handler));
    do_io(handler);
  }
  
  void on_read(server_handler_t& handler, size_t bytes_transferred)
  {
    status_.set(BAS_STATE_ON_READ, bytes_transferred);
    io_buffer(handler).produce(bytes_transferred);
    biz_->process(status_, io_buffer(handler), io_buffer(handler));
    do_io(handler);
  }

  void on_write(server_handler_t& handler, size_t bytes_transferred)
  {
    status_.set(BAS_STATE_ON_WRITE, bytes_transferred);
    io_buffer(handler).consume(bytes_transferred);
    biz_->process(status_, io_buffer(handler), io_buffer(handler));
    do_io(handler);
  }

  void on_close(server_handler_t& handler, const boost::system::error_code& ec)
  {
    if (client_handler_ != 0)
    {
      client_handler_->parent_post(bas::event(bas::event::close));
      client_handler_ = 0;
    }

    status_.set(BAS_STATE_ON_CLOSE, 0, ec);
    biz_->process(status_, io_buffer(handler), io_buffer(handler));
    status_.set(BAS_STATE_NONE);
  }

  void on_parent(server_handler_t& handler, const event_t event)
  {
  }

  void on_child(server_handler_t& handler, const event_t event)
  {
    switch (event.state)
    {
      case bas::event::open:
        status_.set(BAS_STATE_ON_CLIENT_OPEN);
        biz_->process(status_, io_buffer(handler), io_buffer(handler));
        do_io(handler);
        break;

      case bas::event::read:
        status_.set(BAS_STATE_ON_CLIENT_READ, event.value, event.ec);
        // Process should call io_buffer(handler).clear() when need.
        biz_->process(status_, io_buffer(*client_handler_), io_buffer(handler));
        do_io(handler);
        break;

      case bas::event::write:
        status_.set(BAS_STATE_ON_CLIENT_WRITE, event.value, event.ec);
        biz_->process(status_, io_buffer(handler), io_buffer(handler));
        do_io(handler);
        break;

      case bas::event::close:
        client_handler_ = 0;
        status_.set(BAS_STATE_ON_CLIENT_CLOSE, event.value, event.ec);
        biz_->process(status_, io_buffer(handler), io_buffer(handler));
        do_io(handler);
        break;
    }
  }

private:
  /// The I/O status.
  status_t status_;

  /// The business object.
  biz_ptr biz_;

  /// The client object.
  client_ptr client_;

  /// The client handler.
  client_handler_t* client_handler_;
};

} // namespace bastool

#endif // BASTOOL_SERVER_WORK_HPP
