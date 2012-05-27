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
  /// Define type reference of boost::asio::io_service.
  typedef boost::asio::io_service io_service_t;

  /// Define type reference of boost::asio::ip::tcp::endpoint.
  typedef boost::asio::ip::tcp::endpoint endpoint_t;

  /// The type of the service_handler.
  typedef service_handler<Work_Handler, Socket_Service> service_handler_t;
  typedef boost::shared_ptr<service_handler_t> service_handler_ptr;

  /// The type of the service_handler_pool.
  typedef service_handler_pool<Work_Handler, Work_Allocator, Socket_Service> service_handler_pool_t;
  typedef boost::shared_ptr<service_handler_pool_t> service_handler_pool_ptr;

  /// Construct the client object with given endpoint.
  client(service_handler_pool_t* service_handler_pool,
      endpoint_t& peer_endpoint,
      endpoint_t& local_endpoint = endpoint_t())
    : service_handler_pool_(service_handler_pool),
      peer_endpoint_(peer_endpoint),
      local_endpoint_(local_endpoint)
  {
    BOOST_ASSERT(service_handler_pool != 0);

    // Create preallocated handlers of the pool.
    service_handler_pool_->init();
  }

  /// Construct the client object.
  client(service_handler_pool_t* service_handler_pool)
    : service_handler_pool_(service_handler_pool)
  {
    BOOST_ASSERT(service_handler_pool != 0);

    // Create preallocated handlers of the pool.
    service_handler_pool_->init();
  }

  /// Destruct the client object.
   ~client()
  {
    // Release all handlers in the pool.
    service_handler_pool_->close();

    // Destroy service_handler pool.
    service_handler_pool_.reset();
  }

  /// Establish a connection with given io_service and work_service.
  bool connect(io_service_t& io_service,
      io_service_t& work_service,
      endpoint_t& peer_endpoint,
      endpoint_t& local_endpoint = endpoint_t())
  {
    // Get new handler for connect.
    service_handler_ptr new_handler = service_handler_pool_->get_service_handler(io_service,
        work_service);

    if (new_handler.get() == 0)
      return false;

    // Use new handler to connect.
    new_handler->connect(peer_endpoint, local_endpoint);

    return true;
  }

  /// Establish a connection with given io_service and work_service and per_conection_data.
  template<typename Per_connection_data>
  bool connect(io_service_t& io_service,
      io_service_t& work_service,
      Per_connection_data& data,
      endpoint_t& peer_endpoint,
      endpoint_t& local_endpoint = endpoint_t())
  {
   // Get new handler for connect.
    service_handler_ptr new_handler = service_handler_pool_->get_service_handler(io_service,
        work_service);

    if (new_handler.get() == 0)
      return false;

    // Use new handler to connect.
    new_handler->connect(data, peer_endpoint, local_endpoint);

    return true;
  }

  /// Establish a connection with the given parent_handler.
  template<typename Parent_Handler>
  bool connect(Parent_Handler& parent_handler,
      endpoint_t& peer_endpoint,
      endpoint_t& local_endpoint = endpoint_t())
  {
    // Get new handler for connect.
    service_handler_ptr new_handler = service_handler_pool_->get_service_handler(parent_handler.io_service(),
        parent_handler.work_service());

    if (new_handler.get() == 0)
      return false;

    // Execute in work_thread, because connect will be called in the same thread.
    parent_handler.set_child(new_handler.get());
    new_handler->set_parent(&parent_handler);
    
    // Use new handler to connect.
    new_handler->connect(peer_endpoint, local_endpoint);

    return true;
  }

  /// Establish a connection with the given parent_handler and per_conection_data.
  template<typename Parent_Handler, typename Per_connection_data>
  bool connect(Parent_Handler& parent_handler,
      Per_connection_data& data,
      endpoint_t& peer_endpoint,
      endpoint_t& local_endpoint = endpoint_t())
  {
    // Get new handler for connect.
    service_handler_ptr new_handler = service_handler_pool_->get_service_handler(parent_handler.io_service(),
        parent_handler.work_service());

    if (new_handler.get() == 0)
      return false;

    // Execute in work_thread, because connect will be called in the same thread.
    parent_handler.set_child(new_handler.get());
    new_handler->set_parent(&parent_handler);
    
    // Use new handler to connect.
    new_handler->connect(data, peer_endpoint, local_endpoint);

    return true;
  }

  /// Establish a connection with given io_service and work_service.
  bool connect(io_service_t& io_service,
      io_service_t& work_service)
  {
    // Connect with the internal endpoint.
    return connect(io_service, work_service, peer_endpoint_, local_endpoint_);
  }

  /// Establish a connection with given io_service and work_service and per_conection_data.
  template<typename Per_connection_data>
  bool connect(io_service_t& io_service,
      io_service_t& work_service,
      Per_connection_data& data)
  {
    // Connect with the internal endpoint.
    return connect(io_service, work_service, data, peer_endpoint_, local_endpoint_);
  }

  /// Establish a connection with the given parent_handler.
  template<typename Parent_Handler>
  bool connect(Parent_Handler& parent_handler)
  {
    // Connect with the internal endpoint.
    return connect(parent_handler, peer_endpoint_, local_endpoint_);
  }

  /// Establish a connection with the given parent_handler and per_conection_data.
  template<typename Parent_Handler, typename Per_connection_data>
  bool connect(Parent_Handler& parent_handler,
      Per_connection_data& data)
  {
    // Connect with the internal endpoint.
    return connect(parent_handler, data, peer_endpoint_, local_endpoint_);
  }

private:
  /// The pool of service_handler objects.
  service_handler_pool_ptr service_handler_pool_;

  /// The client endpoint.
  endpoint_t local_endpoint_;

  /// The server endpoint.
  endpoint_t peer_endpoint_;
};

} // namespace bas

#endif // BAS_CLIENT_HPP
