//
// ssl_server_work.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2009 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BAS_SSL_SERVER_WORK_HPP
#define BAS_SSL_SERVER_WORK_HPP

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include <bas/service_handler.hpp>

#include <iostream>

namespace echo {

class ssl_server_work
{
public:
  typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;
  typedef bas::service_handler<ssl_server_work, ssl_socket> server_handler_type;
  typedef boost::shared_ptr<server_handler_type> server_handler_ptr;

  ssl_server_work()
  {
  }
  
  void clear()
  {
  }
  
  void on_open(server_handler_type& handler)
  {
    handler.socket().async_handshake(boost::asio::ssl::stream_base::server,
        boost::bind(&ssl_server_work::handle_handshake,
            this,
            handler.shared_from_this(),
            boost::asio::placeholders::error));
  }

  void handle_handshake(server_handler_ptr handler_ptr, const boost::system::error_code& e)
  {
    if (!e)
    {
      handler_ptr->async_read_some();
    }
    else
    {
      handler_ptr->close(e);
    }
  }

  void on_read(server_handler_type& handler, std::size_t bytes_transferred)
  {
    handler.async_write(boost::asio::buffer(handler.read_buffer().data(), bytes_transferred));
  }

  void on_write(server_handler_type& handler)
  {
    handler.close();
  }

  void on_close(server_handler_type& handler, const boost::system::error_code& e)
  {
    switch (e.value())
    {
      // Operation successfully completed.
      case 0:
      case boost::asio::error::eof:
        break;

      // Connection breaked.
      case boost::asio::error::connection_aborted:
      case boost::asio::error::connection_reset:
      case boost::asio::error::connection_refused:
        break;

      // Other error.
      case boost::asio::error::timed_out:
      case boost::asio::error::no_buffer_space:
      default:
        std::cout << "server error " << e << " message " << e.message() << "\n";
        std::cout.flush();
        break;
    }
  }

  void on_parent(server_handler_type& handler, const bas::event event)
  {
  }

  void on_child(server_handler_type& handler, const bas::event event)
  {
  }
};

} // namespace echo

#endif // BAS_SSL_SERVER_WORK_HPP
