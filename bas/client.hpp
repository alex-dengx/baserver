//
// client.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2009 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BAS_CLIENT_HPP
#define BAS_CLIENT_HPP

#include <boost/asio/detail/mutex.hpp>
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>

#include <bas/io_service_pool.hpp>
#include <bas/service_handler.hpp>
#include <bas/service_handler_pool.hpp>

namespace bas {

/// The top-level class of the client.
template<typename Work_Handler, typename Work_Allocator, typename Socket_Service = boost::asio::ip::tcp::socket>
class client
  : private boost::noncopyable
{
public:
  /// The type of the service_handler.
  typedef service_handler<Work_Handler, Socket_Service> service_handler_type;
  typedef boost::shared_ptr<service_handler_type> service_handler_ptr;

  /// The type of the service_handler_pool.
  typedef service_handler_pool<Work_Handler, Work_Allocator, Socket_Service> service_handler_pool_type;
  typedef boost::shared_ptr<service_handler_pool_type> service_handler_pool_ptr;

  /// Construct the client object for connect to specified TCP address and port.
  explicit client(const std::string& address, const std::string& port,
      service_handler_pool_type* service_handler_pool)
    : mutex_(),
      service_handler_pool_(service_handler_pool),
      endpoint_iterator_()
  {
    BOOST_ASSERT(service_handler_pool != 0);

    // Prepare endpoint_iterator for connect.
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::resolver resolver(io_service);
    boost::asio::ip::tcp::resolver::query query(address, port);
    endpoint_iterator_ = resolver.resolve(query);
  }

  /// Destruct the client object.
   ~client()
  {
    // Destroy service_handler pool.
    service_handler_pool_.reset();
  }

  /// Make an connection with given io_service and work_service.
  void connect(boost::asio::io_service& io_service, boost::asio::io_service& work_service)
  {
    // Get new handler for connect.
    service_handler_ptr new_handler = service_handler_pool_->get_service_handler(io_service,
        work_service,
        mutex_);
    // Use new handler to connect.
    new_handler->connect(endpoint_iterator_);
  }

  /// Make an connection with the given parent_handler.
  template<typename Parent_Handler>
  void connect(Parent_Handler* parent_handler)
  {
    BOOST_ASSERT(parent_handler != 0);

    // Get new handler for connect.
    service_handler_ptr new_handler = service_handler_pool_->get_service_handler(parent_handler->io_service(),
        parent_handler->work_service(),
        mutex_);
    // Use new handler to connect.
    new_handler->connect(endpoint_iterator_, parent_handler);
  }

private:
  /// Mutex to protect access to internal data.
  boost::asio::detail::mutex mutex_;

  /// The pool of service_handler objects.
  service_handler_pool_ptr service_handler_pool_;

  /// The server endpoint iterator.
  boost::asio::ip::tcp::resolver::iterator endpoint_iterator_;
};

} // namespace bas

#endif // BAS_CLIENT_HPP
