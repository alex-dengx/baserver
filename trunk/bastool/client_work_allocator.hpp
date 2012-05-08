//
// client_work_allocator.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2009, 2012 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BASTOOL_CLIENT_WORK_ALLOCATOR_HPP
#define BASTOOL_CLIENT_WORK_ALLOCATOR_HPP

#include <bastool/client_work.hpp>

namespace bastool {

template<typename Biz_Handler>
class client_work_allocator
{
public:
  typedef client_work<Biz_Handler> client_work_t;
  typedef boost::asio::ip::tcp::socket socket_t;

  /// Constructor.
  client_work_allocator()
  {
  }

  socket_t* make_socket(boost::asio::io_service& io_service)
  {
    return new socket_t(io_service);
  }

  client_work_t* make_handler()
  {
    return new client_work_t();
  }
};

} // namespace bastool

#endif // BASTOOL_CLIENT_WORK_ALLOCATOR_HPP
