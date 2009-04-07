//
// server_work.hpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2009 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BAS_HTTP_SERVER_WORK_HPP
#define BAS_HTTP_SERVER_WORK_HPP

#include <boost/assert.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <bas/service_handler.hpp>

#include <iostream>

#include "request_handler.hpp"
#include "request_parser.hpp"
#include "request.hpp"
#include "reply.hpp"

namespace http {
namespace server {  

class server_work
{
public:

  typedef bas::service_handler<server_work> server_handler_type;

  server_work(request_handler& handler)
    : request_handler_(handler)
  {
  }
  
  void clear()
  {
    request_.reset();
    request_parser_.reset();
    reply_.reset();
  }

  void on_open(server_handler_type& handler)
  {
    handler.async_read_some();
  }

  void on_read(server_handler_type& handler, std::size_t bytes_transferred)
  {
    boost::tribool result;
    boost::tie(result, boost::tuples::ignore) = request_parser_.parse(
        request_, handler.read_buffer().data(), handler.read_buffer().data() + bytes_transferred);

    if (result)
    {
      request_handler_.handle_request(request_, reply_);
      handler.async_write(reply_.to_buffers());
    }
    else if (!result)
    {
      reply_ = reply::stock_reply(reply::bad_request);
      handler.async_write(reply_.to_buffers());
    }
    else
    {
      handler.read_buffer().clear();
      clear();

      handler.async_read_some();
    }
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

private:
  /// The handler used to process the incoming request.
  request_handler& request_handler_;

  /// The incoming request.
  request request_;

  /// The parser for the incoming request.
  request_parser request_parser_;

  /// The reply to be sent back to the client.
  reply reply_;
};

} // namespace server
} // namespace http

#endif // BAS_HTTP_SERVER_WORK_HPP
