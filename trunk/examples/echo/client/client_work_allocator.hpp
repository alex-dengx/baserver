//
// client_work_allocator.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2009 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BAS_ECHO_CLIENT_WORK_ALLOCATOR_HPP
#define BAS_ECHO_CLIENT_WORK_ALLOCATOR_HPP

#include "client_work.hpp"
#include "error_count.hpp"

namespace echo {

class client_work_allocator
{
public:
  typedef boost::asio::ip::tcp::socket socket_type;

  client_work_allocator(error_count* counter, unsigned int pause_time)
    : error_count_(counter),
      pause_time_(pause_time)
  {
  }

  socket_type* make_socket(boost::asio::io_service& io_service)
  {
    return new socket_type(io_service);
  }

  client_work* make_handler()
  {
    return new client_work(error_count_, pause_time_);
  }

private:
  error_count* error_count_;
  unsigned int pause_time_;
};

} // namespace echo

#endif // BAS_ECHO_CLIENT_WORK_ALLOCATOR_HPP
