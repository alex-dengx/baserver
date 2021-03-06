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

template<typename Biz_Handler, typename Biz_Global_Storage>
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
  typedef boost::shared_ptr<Biz_Global_Storage> bgs_ptr;
  typedef boost::shared_ptr<client_t> client_ptr;

  /// Constructor.
  server_work_allocator(Biz_Global_Storage* bgs = 0,
                        client_t* client = 0)
    : bgs_(bgs),
      client_(client)
  {
    /// Open and allocate resource in bgs.
    if (bgs_.get() != 0)
      bgs_->init();
  }
  
  /// Destructor.
  ~server_work_allocator()
  {
    /// Close and release resource in bgs.
    if (bgs_.get() != 0)
      bgs_->close();
    
    /// Destroy bgs object.
    bgs_.reset();

    /// Destroy client object.
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
  /// Business Global storage for holding application resources.
  bgs_ptr bgs_;

  /// The client object.
  client_ptr client_;
};

} // namespace bastool

#endif // BASTOOL_SERVER_WORK_ALLOCATOR_HPP
