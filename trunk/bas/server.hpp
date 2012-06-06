//
// server.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2009 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BAS_SERVER_HPP
#define BAS_SERVER_HPP

#include <boost/assert.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include <bas/io_service_group.hpp>
#include <bas/service_handler.hpp>
#include <bas/service_handler_pool.hpp>

namespace bas {

#define BAS_ACCEPT_QUEUE_LENGTH   250
#define BAS_ACCEPT_DELAY_SECONDS  1

/// The top-level class of the server.
template<typename Work_Handler, typename Work_Allocator, typename Socket_Service = boost::asio::ip::tcp::socket>
class server
  : private boost::noncopyable
{
public:
  /// Define type reference of std::size_t.
  typedef std::size_t size_type;

  /// Define type reference of boost::asio::ip::tcp::endpoint.
  typedef boost::asio::ip::tcp::endpoint endpoint_t;

  /// The type of the service_handler.
  typedef service_handler<Work_Handler, Socket_Service> service_handler_t;
  typedef boost::shared_ptr<service_handler_t> service_handler_ptr;

  /// The type of the service_handler_pool.
  typedef service_handler_pool<Work_Handler, Work_Allocator, Socket_Service> service_handler_pool_t;
  typedef boost::shared_ptr<service_handler_pool_t> service_handler_pool_ptr;

  typedef boost::shared_ptr<io_service_group> io_service_group_ptr;

  /// Construct server object with internal io_service_group..
  server(service_handler_pool_t* service_handler_pool,
      endpoint_t& local_endpoint,
      size_t io_pool_size = BAS_IO_SERVICE_POOL_INIT_SIZE,
      size_t work_pool_init_size = BAS_IO_SERVICE_POOL_INIT_SIZE,
      size_t work_pool_high_watermark = BAS_IO_SERVICE_POOL_HIGH_WATERMARK,
      size_t work_pool_thread_load = BAS_IO_SERVICE_POOL_THREAD_LOAD,
      size_t accept_queue_length = BAS_ACCEPT_QUEUE_LENGTH)
    : service_handler_pool_(service_handler_pool),
      acceptor_service_pool_(1),
      acceptor_(acceptor_service_pool_.get_io_service()),
      timer_(acceptor_.get_io_service()),
      endpoint_(local_endpoint),
      accept_queue_length_(accept_queue_length),
      started_(false),
      block_(false),
      service_group_(new io_service_group(2)),
      has_service_group_(true)
  {
    BOOST_ASSERT(service_handler_pool != 0);
    BOOST_ASSERT(accept_queue_length != 0);

    service_group_->get(io_service_group::io_pool).set(io_pool_size, io_pool_size);
    service_group_->get(io_service_group::work_pool).set(work_pool_init_size, work_pool_high_watermark, work_pool_thread_load);

    // Create preallocated handlers of the pool.
    service_handler_pool_->init();
  }

  /// Construct server object with external io_service_group.
  server(service_handler_pool_t* service_handler_pool,
      endpoint_t& local_endpoint,
      size_t accept_queue_length = BAS_ACCEPT_QUEUE_LENGTH)
    : service_handler_pool_(service_handler_pool),
      acceptor_service_pool_(1),
      acceptor_(acceptor_service_pool_.get_io_service()),
      timer_(acceptor_.get_io_service()),
      endpoint_(local_endpoint),
      accept_queue_length_(accept_queue_length),
      started_(false),
      block_(false),
      service_group_(),
      has_service_group_(false)
  {
    BOOST_ASSERT(service_handler_pool != 0);
    BOOST_ASSERT(accept_queue_length != 0);

    // Create preallocated handlers of the pool.
    service_handler_pool_->init();
  }

  /// Destructor.
  ~server()
  {
    // Stop server.
    stop();

    // Destroy instance of io_service_group.
    service_group_.reset();

    // Release all handlers in the pool.
    service_handler_pool_->close();

    // Destroy service_handler pool.
    service_handler_pool_.reset();
  }

  /// Set stop mode to graceful or force.
  server& set(bool force_stop = false)
  {
    if (!started_ && service_group_.get() != 0)
      service_group_->set(force_stop);

    return *this;
  }

  /// Set io_service_group to use.
  server& set(io_service_group_ptr& service_group)
  {
    if (!started_ && service_group.get() != 0)
    {
      service_group_.reset();
      service_group_ = service_group;
      has_service_group_ = false;
    }

    return *this;
  }

  /// Start server with non-blocked model.
  void start()
  {
    start(false);
  }

  /// Run server with blocked model.
  void run()
  {
    start(true);
  }

  /// Stop server.
  void stop()
  {
    if (!started_                 || \
        service_group_.get() == 0 || \
        !has_service_group_ && !service_group_->started())
      return;

    // Close the acceptor in the same thread.
    acceptor_.get_io_service().dispatch(boost::bind(&boost::asio::ip::tcp::acceptor::close,
        &acceptor_));

    // Stop accept_service_pool.
    acceptor_service_pool_.stop();

    if (!block_)
    {
      // Stop internal io_service_group.
      if (has_service_group_)
        service_group_->stop();

      started_ = false;
    }
  }

private:
  /// Start server with given mode.
  void start(bool block)
  {
    if (started_                  || \
        service_group_.get() == 0 || \
        !has_service_group_ && !service_group_->started())
      return;

    // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
    acceptor_.open(endpoint_.protocol());
    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));

    boost::system::error_code e;
    acceptor_.bind(endpoint_, e);
    if (e)
      return;

    acceptor_.listen();
  
    // Accept new connections.
    for (size_t i = 0; i < accept_queue_length_; ++i)
      accept_one();

    // Start internal io_service_group with non-blocked mode.
    if (has_service_group_)
      service_group_->start();

    block_ = block;

    if (block)
    {
      started_ = true;

      // Start accept_service_pool with blocked mode.
      acceptor_service_pool_.run();

      // Stop internal io_service_group.
      if (has_service_group_)
        service_group_->stop();

      started_ = false;
    }
    else
    {
      // Start accept_service_pool with non-blocked mode.
      acceptor_service_pool_.start();

      started_ = true;
    }
  }

  /// Start an asynchronous accept, can be call from any thread.
  void accept_one()
  {
    acceptor_.get_io_service().dispatch(boost::bind(&server::accept_one_i,
        this));
  }

  /// Start an asynchronous accept in io_service thread.
  void accept_one_i()
  {
    // Get new handler for accept.
    service_handler_ptr handler = service_handler_pool_->get_service_handler(service_group_->get(io_service_group::io_pool).get_io_service(),
            service_group_->get(io_service_group::work_pool).get_io_service(service_handler_pool_->get_load()));

    // Wait for some seconds to accept next connection if exceed max connection number.
    if (handler.get() == 0)
    {
      timer_.expires_from_now(boost::posix_time::seconds(BAS_ACCEPT_DELAY_SECONDS));
      timer_.async_wait(boost::bind(&server::handle_timeout,
          this,
          boost::asio::placeholders::error));
      
      return;
    }

    // Use new handler to accept.
    acceptor_.async_accept(handler->socket().lowest_layer(),
        boost::bind(&server::handle_accept,
            this,
            boost::asio::placeholders::error,
            handler));
  }

  /// Handle completion of an asynchronous accept operation.
  void handle_accept(const boost::system::error_code& e,
      service_handler_ptr handler)
  {
    if (!e)
    {
      // Start the first operation of the current handler.
      handler->start();

      // Accept new connection in io_service thread.
      accept_one_i();
    }
    else
    {
      if (e == boost::asio::error::operation_aborted)
      {
        // The acceptor has been normally stopped.
        handler->close();
      }
      else
        handler->close(e);
    }
  }

  /// Handle timeout of wait for repeat accept.
  void handle_timeout(const boost::system::error_code& e)
  {
    // The timer has been cancelled, do nothing.
    if (e == boost::asio::error::operation_aborted)
      return;

     // Accept new connection in io_service thread.
    accept_one_i();
  }

private:
  /// The pool of service_handler objects.
  service_handler_pool_ptr service_handler_pool_;

  /// The group of io_service_pool objects used to perform asynchronous operations.
  io_service_group_ptr service_group_;

  /// The pool of io_service objects used to perform asynchronous accept operations.
  io_service_pool acceptor_service_pool_;

  /// The acceptor used to listen for incoming connections.
  boost::asio::ip::tcp::acceptor acceptor_;

  /// The timer for repeat accept delay.
  boost::asio::deadline_timer timer_;

  /// The server endpoint.
  endpoint_t endpoint_;

  /// The queue length for async_accept.
  size_t accept_queue_length_;

  /// Flag to indicate whether the server is started.
  bool started_;

  /// Flag to indicate whether the server is started with blocked mode.
  bool block_;  

  /// Flag to indicate whether the server has internal io_service_group.
  bool has_service_group_;
};

} // namespace bas

#endif // BAS_SERVER_HPP
