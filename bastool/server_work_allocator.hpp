//
// server_work_allocator.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2009, 2012 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BASTOOL_SERVER_WORK_ALLOCATOR_HPP
#define BASTOOL_SERVER_WORK_ALLOCATOR_HPP

#include <boost/shared_ptr.hpp>
#include <bas/service_handler_pool.hpp>
#include <bas/client.hpp>
#include <bastool/client_work.hpp>
#include <bastool/client_work_allocator.hpp>
#include <bastool/server_work.hpp>

namespace bastool {

using namespace bas;

template<typename Global_Storage, typename Biz_Handler>
class server_work_allocator
{
public:
  /// Define type reference of handlers and allocators.
  typedef server_work<Biz_Handler> server_work_t;
  typedef client_work<Biz_Handler> client_work_t;
  typedef client_work_allocator<Biz_Handler> client_work_allocator_t;
  typedef client<client_work_t, client_work_allocator_t> client_t;
  typedef service_handler_pool<client_work_t, client_work_allocator_t> client_handler_pool_t;
  typedef boost::asio::ip::tcp::socket socket_t;

  /// Constructor.
  server_work_allocator(Global_Storage& bgs,
                        client_t* client = 0)
    : bgs_(bgs),
      client_(client)
  {
  }
  
  /// Destructor.
  ~server_work_allocator()
  {
    client_.reset();
  }

  socket_t* make_socket(boost::asio::io_service& io_service)
  {
    return new socket_t(io_service);
  }

  server_work_t* make_handler()
  {
    return new server_work_t(new Biz_Handler(bgs_), client_);
  }

private:
  /// The global storage can be used in process for holding other application resources.
  Global_Storage& bgs_;

  /// The client object.
  boost::shared_ptr<client_t> client_;
};

} // namespace bastool

#endif // BASTOOL_SERVER_WORK_ALLOCATOR_HPP
