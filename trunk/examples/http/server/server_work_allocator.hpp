//
// server_work_allocator.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2009 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BAS_HTTP_SERVER_WORK_ALLOCATOR_HPP
#define BAS_HTTP_SERVER_WORK_ALLOCATOR_HPP

#include "request_handler.hpp"
#include "server_work.hpp"

namespace http {
namespace server {

class server_work_allocator
{
public:
  typedef boost::asio::ip::tcp::socket socket_type;

  server_work_allocator(const std::string& doc_root)
   : request_handler_(doc_root)
  {
  }

  socket_type* make_socket(boost::asio::io_service& io_service)
  {
    return new socket_type(io_service);
  }

  server_work* make_handler()
  {
    return new server_work(request_handler_);
  }

private:
  /// The handler for all incoming requests.
  request_handler request_handler_;
};

} // namespace server
} // namespace http

#endif // BAS_HTTP_SERVER_WORK_ALLOCATOR_HPP
