//
// client_work.hpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2009, 2012 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BASTOOL_CLIENT_WORK_HPP
#define BASTOOL_CLIENT_WORK_HPP

#include <bas/service_handler.hpp>

namespace bastool {

using namespace bas;

template<typename Biz_Handler>
class server_work;

/// Object for handle client asynchronous operations.
template<typename Biz_Handler>
class client_work
{
public:
  /// Define type reference of std::size_t.
  typedef std::size_t size_t;

  /// Define type reference of handlers.
  typedef server_work<Biz_Handler> server_work_t;
  typedef client_work<Biz_Handler> client_work_t;
  typedef service_handler<server_work_t> server_handler_t;
  typedef service_handler<client_work_t> client_handler_t;

  /// Constructor.
  client_work()
  : server_handler_(0),
    event_(),
    passive_close_(false)
  {
  }
  
  /// Quick reference to read_buffer for normally used.
  template<typename Service_Handler>
  bas::io_buffer& io_buffer(Service_Handler& handler)
  {
    return handler.read_buffer();
  }
  
  void on_set_parent(client_handler_t& handler, server_handler_t* server_handler)
  {
    BOOST_ASSERT(server_handler != 0);
    
    passive_close_ = false;
    server_handler_ = server_handler;
  }

  void on_clear(client_handler_t& handler)
  {
  }
  
  void on_open(client_handler_t& handler)
  {
    BOOST_ASSERT(server_handler_ != 0);

    io_buffer(handler).clear();
    server_handler_->child_post(bas::event(bas::event::open));
  }

  void on_read(client_handler_t& handler, size_t bytes_transferred)
  {
    BOOST_ASSERT(server_handler_ != 0);

    io_buffer(handler).produce(bytes_transferred);
    server_handler_->child_post(bas::event(bas::event::read, bytes_transferred));
  }

  void on_write(client_handler_t& handler, size_t bytes_transferred)
  {
    BOOST_ASSERT(server_handler_ != 0);

    io_buffer(handler).consume(bytes_transferred);
    io_buffer(handler).crunch();

    if (event_.state == bas::event::write_read)
    {
      handler.async_read_some();
    }
    else
      server_handler_->child_post(bas::event(bas::event::write, bytes_transferred));
  }

  void on_close(client_handler_t& handler, const boost::system::error_code& ec)
  {
    if (server_handler_ != 0)
    {
      // Notify parent to close.
      if (!passive_close_)
        server_handler_->child_post(bas::event(bas::event::close, 0, ec));

      server_handler_ = 0;
    }
  }
  
  void on_parent(client_handler_t& handler, const event_t event)
  {
    BOOST_ASSERT(server_handler_ != 0);

    event_ = event;

    switch (event_.state)
    {
      case bas::event::close:
        // The client_handler is requesting to close by server_handler.
        passive_close_ = true;
        handler.close();
        break;

      case bas::event::write:
      case bas::event::write_read:
        handler.async_write(boost::asio::buffer(io_buffer(handler).data(), io_buffer(handler).size()));
        break;

      case bas::event::read:
        handler.async_read_some();
        break;
    }
  }

  void on_child(client_handler_t& handler, const event_t e)
  {
  }

private:
  /// The server handler.
  server_handler_t* server_handler_;
  
  /// The event from server_handler.
  event_t event_;

  /// Flag for passive close.
  bool passive_close_;
};

} // namespace bastool

#endif // BASTOOL_CLIENT_WORK_HPP
