//
// ssl_client_work.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2009 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BAS_ECHO_SSL_CLIENT_WORK_HPP
#define BAS_ECHO_SSL_CLIENT_WORK_HPP

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include <bas/service_handler.hpp>

#include <iostream>

namespace echo {

std::string echo_message = "echo server test message.\r\n";

class ssl_client_work
{
public:
  typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;
  typedef bas::service_handler<ssl_client_work, ssl_socket> client_handler_type;
  typedef boost::shared_ptr<client_handler_type> client_handler_ptr;

  ssl_client_work()
  {
  }

  void clear()
  {
  }

  void on_open(client_handler_type& handler)
  {
    handler.socket().async_handshake(boost::asio::ssl::stream_base::client,
        boost::bind(&ssl_client_work::handle_handshake,
            this,
            handler.shared_from_this(),
            boost::asio::placeholders::error));
  }

  void handle_handshake(client_handler_ptr handler_ptr, const boost::system::error_code& e)
  {
    if (!e)
    {
      handler_ptr->async_write(boost::asio::buffer(echo_message));
    }
    else
    {
      handler_ptr->close(e);
    }
  }

  void on_read(client_handler_type& handler, std::size_t bytes_transferred)
  {
    handler.close();
  }

  void on_write(client_handler_type& handler)
  {
    handler.async_read_some();
  }

  void on_close(client_handler_type& handler, const boost::system::error_code& e)
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
        std::cout << "client error " << e << " message " << e.message() << "\n";
        std::cout.flush();
        break;
    }
  }

  void on_parent(client_handler_type& handler, const bas::event event)
  {
  }

  void on_child(client_handler_type& handler, const bas::event event)
  {
  }
};

} // namespace echo

#endif // BAS_ECHO_SSL_CLIENT_WORK_HPP
