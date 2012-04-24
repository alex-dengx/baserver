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

#include <bas/io_service_pool.hpp>
#include <bas/service_handler.hpp>
#include <bas/service_handler_pool.hpp>

namespace bas {

#define BAS_ACCEPT_QUEUE_LENGTH  250

/// The top-level class of the server.
template<typename Work_Handler, typename Work_Allocator, typename Socket_Service = boost::asio::ip::tcp::socket>
class server
  : private boost::noncopyable
{
public:
  /// The type of the service_handler.
  typedef service_handler<Work_Handler, Socket_Service> service_handler_type;
  typedef boost::shared_ptr<service_handler_type> service_handler_ptr;

  /// The type of the service_handler_pool.
  typedef service_handler_pool<Work_Handler, Work_Allocator, Socket_Service> service_handler_pool_type;
  typedef boost::shared_ptr<service_handler_pool_type> service_handler_pool_ptr;

  /// Construct the server to listen on the specified TCP address and port.
  explicit server(service_handler_pool_type* service_handler_pool,
      const std::string& address,
      unsigned short port,
      std::size_t io_pool_size = BAS_IO_SERVICE_POOL_INIT_SIZE,
      std::size_t work_pool_init_size = BAS_IO_SERVICE_POOL_INIT_SIZE,
      std::size_t work_pool_high_watermark = BAS_IO_SERVICE_POOL_HIGH_WATERMARK,
      std::size_t work_pool_thread_load = BAS_IO_SERVICE_POOL_THREAD_LOAD,
      std::size_t accept_queue_length = BAS_ACCEPT_QUEUE_LENGTH)
    : service_handler_pool_(service_handler_pool),
      acceptor_service_pool_(1),
      io_service_pool_(io_pool_size),
      work_service_pool_(work_pool_init_size, work_pool_high_watermark, work_pool_thread_load),
      acceptor_(acceptor_service_pool_.get_io_service()),
      endpoint_(boost::asio::ip::address::from_string(address), port),
      accept_queue_length_(accept_queue_length),
      started_(false),
      block_(false),
      force_stop_(false)
  {
    BOOST_ASSERT(service_handler_pool != 0);
    BOOST_ASSERT(accept_queue_length != 0);

    // Create preallocated handlers of the pool.
    service_handler_pool->init();
  }

  /// Destruct the server object.
  ~server()
  {
    // Stop the server's io_service loop.
    stop();

    // Release preallocated handler in the pool.
    service_handler_pool_->close();

    // Destroy service_handler pool.
    service_handler_pool_.reset();
  }
  
  /// Set stop with gracefully mode or force mode.
  void set_stop_mode(bool force_stop = false)
  {
    force_stop_ = force_stop;
  }

  /// Start server in nonblock model.
  void start()
  {
    start(false);
  }

  /// Run server in block model.
  void run()
  {
    start(true);
  }

  /// Stop the server.
  void stop()
  {
    if (!started_)
      return;

    // Close the acceptor in the same thread.
    acceptor_.get_io_service().dispatch(boost::bind(&boost::asio::ip::tcp::acceptor::close,
        &acceptor_));

    // Stop accept_service_pool from block.
    acceptor_service_pool_.stop();

    if (!block_)
    {
      stop_pool();

      started_ = false;
    }
  }

private:

  /// Start server with given mode.
  void start(bool block)
  {
    if (started_)
      return;

    // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
    acceptor_.open(endpoint_.protocol());
    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint_);
    acceptor_.listen();
  
    // Accept new connections.
    for (std::size_t i = 0; i < accept_queue_length_; ++i)
      accept_one();

    // Start work_service_pool with nonblock to perform synchronous works.
    work_service_pool_.start();
    // Start io_service_pool with nonblock to perform asynchronous i/o operations.
    io_service_pool_.start();

    block_ = block;

    // Start accept_service_pool with block to perform asynchronous accept operations.
    if (block)
    {
      started_ = true;

      // Start accept_service_pool with block to perform asynchronous accept operations.
      acceptor_service_pool_.run();

      stop_pool();

      started_ = false;
    }
    else
    {
      acceptor_service_pool_.start();

      started_ = true;
    }
  }

  /// Wait for all io_service_pool to exit.
  void stop_pool()
  {
    if (force_stop_)
    {
      // Stop io_service_pool with force mode.
      io_service_pool_.stop(force_stop_);
      // Stop work_service_pool with force mode.
      work_service_pool_.stop(force_stop_);
    }
    else
    {
      // Stop io_service_pool.
      io_service_pool_.stop();
      // Stop work_service_pool.
      work_service_pool_.stop();

      // For gracefully close, continue to repeat several times to dispatch and perform asynchronous operations/handlers.
      while (!io_service_pool_.is_free() || !work_service_pool_.is_free())
      {
        work_service_pool_.start();
        io_service_pool_.start();
        io_service_pool_.stop();
        work_service_pool_.stop();
      }
    }
  }

  /// Start to accept one connection.
  void accept_one()
  {
    // Get new handler for accept.
    service_handler_ptr handler = service_handler_pool_->get_service_handler(io_service_pool_.get_io_service(),
        work_service_pool_.get_io_service(service_handler_pool_->get_load()));

    // Use new handler to accept, and bind with acceptor's io_service.
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

      // Accept new connection.
      accept_one();
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

private:
  /// The pool of service_handler objects.
  service_handler_pool_ptr service_handler_pool_;

  /// The pool of io_service objects used to perform asynchronous accept operations.
  io_service_pool acceptor_service_pool_;

  /// The pool of io_service objects used to perform asynchronous i/o operations.
  io_service_pool io_service_pool_;

  /// The pool of io_service objects used to perform synchronous works.
  io_service_pool work_service_pool_;

  /// The acceptor used to listen for incoming connections.
  boost::asio::ip::tcp::acceptor acceptor_;

  /// The server endpoint.
  boost::asio::ip::tcp::endpoint endpoint_;

  /// The queue length for async_accept.
  std::size_t accept_queue_length_;

  /// Flag to indicate that the server is started or not.
  bool started_;

  /// Flag to indicate that start() functions will block or not.
  bool block_;  

  /// Flag to indicate the server whether with gracefully stop mode.
  bool force_stop_;
};

} // namespace bas

#endif // BAS_SERVER_HPP
