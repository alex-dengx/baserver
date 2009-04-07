//
// server_work_allocator.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2009 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BAS_PROXY_SERVER_WORK_ALLOCATOR_HPP
#define BAS_PROXY_SERVER_WORK_ALLOCATOR_HPP

#include <bas/service_handler_pool.hpp>
#include <bas/client.hpp>

#include "client_work.hpp"
#include "client_work_allocator.hpp"
#include "server_work.hpp"

namespace proxy {

class server_work_allocator
{
public:

  typedef bas::client<client_work, client_work_allocator> client_type;
  typedef bas::service_handler_pool<client_work, client_work_allocator> client_handler_pool_type;
  typedef boost::asio::ip::tcp::socket socket_type;

  server_work_allocator(const std::string& address,
      const std::string& port,
      client_handler_pool_type* client_work_pool)
    : client_(address,
          port,
          client_work_pool)
  {
  }

  socket_type* make_socket(boost::asio::io_service& io_service)
  {
    return new socket_type(io_service);
  }

  server_work* make_handler()
  {
    return new server_work(client_);
  }

private:
  client_type client_;
};

} // namespace proxy

#endif // BAS_PROXY_SERVER_WORK_ALLOCATOR_HPP
