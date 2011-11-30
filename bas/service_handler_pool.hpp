//
// service_handler_pool.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2011 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BAS_SERVICE_HANDLER_POOL_HPP
#define BAS_SERVICE_HANDLER_POOL_HPP

#include <boost/assert.hpp>
#include <boost/asio/detail/mutex.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <vector>

#include <bas/service_handler.hpp>

namespace bas {

#define BAS_HANDLER_POOL_INIT_SIZE       100
#define BAS_HANDLER_POOL_LOW_WATERMARK   0
#define BAS_HANDLER_POOL_HIGH_WATERMARK  200
#define BAS_HANDLER_POOL_INCREMENT       10

#define BAS_HANDLER_BUFFER_DEFAULT_SIZE  256
#define BAS_HANDLER_DEFAULT_TIMEOUT      30

/// A pool of service_handler objects.
template<typename Work_Handler, typename Work_Allocator, typename Socket_Service = boost::asio::ip::tcp::socket>
class service_handler_pool
  : public boost::enable_shared_from_this<service_handler_pool<Work_Handler, Work_Allocator, Socket_Service> >,
    private boost::noncopyable
{
public:
  using boost::enable_shared_from_this<service_handler_pool<Work_Handler, Work_Allocator, Socket_Service> >::shared_from_this;

  /// The type of the service_handler.
  typedef service_handler<Work_Handler, Socket_Service> service_handler_type;
  typedef boost::shared_ptr<service_handler_type> service_handler_ptr;

  /// The type of the work_allocator.
  typedef Work_Allocator work_allocator_type;
  typedef boost::shared_ptr<work_allocator_type> work_allocator_ptr;

  /// Construct the service_handler pool.
  explicit service_handler_pool(work_allocator_type* work_allocator,
      std::size_t pool_init_size = BAS_HANDLER_POOL_INIT_SIZE,
      std::size_t read_buffer_size = BAS_HANDLER_BUFFER_DEFAULT_SIZE,
      std::size_t write_buffer_size = 0,
      std::size_t timeout_seconds = BAS_HANDLER_DEFAULT_TIMEOUT,
      std::size_t pool_low_watermark = BAS_HANDLER_POOL_LOW_WATERMARK,
      std::size_t pool_high_watermark = BAS_HANDLER_POOL_HIGH_WATERMARK,
      std::size_t pool_increment = BAS_HANDLER_POOL_INCREMENT)
    : mutex_(),
      service_handlers_(),
      work_allocator_(work_allocator),
      read_buffer_size_(read_buffer_size),
      write_buffer_size_(write_buffer_size),
      timeout_seconds_(timeout_seconds),
      pool_init_size_(pool_init_size),
      pool_low_watermark_(pool_low_watermark),
      pool_high_watermark_(pool_high_watermark),
      pool_increment_(pool_increment),
      handler_count_(0),
      closed_(false)
  {
    BOOST_ASSERT(work_allocator != 0);
    BOOST_ASSERT(pool_init_size != 0);
    BOOST_ASSERT(pool_low_watermark <= pool_init_size);
    BOOST_ASSERT(pool_high_watermark > pool_low_watermark);
    BOOST_ASSERT(pool_increment != 0);
  }

  /// Destruct the pool object.
  ~service_handler_pool()
  {
    work_allocator_.reset();
  }

  /// Create preallocated handler to the pool.
  /// Note: shared_from_this() can't be used in the constructor.
  void init(void)
  {
    // Create preallocated handler to the pool.
    create_handler(pool_init_size_);
  }

  /// Release preallocated handler in the pool.
  /// Note: close() can't be used in the destructor.
  void close(void)
  {
    // Release preallocated handler in the pool.
    clear();
  }

  /// Get an service_handler to use.
  service_handler_ptr get_service_handler(boost::asio::io_service& io_service,
      boost::asio::io_service& work_service)
  {
    // Get a handler.
    service_handler_ptr service_handler = get_handler();

    // Bind the handler with given io_service and work_service.
    service_handler->bind(io_service, work_service, work_allocator());
    return service_handler;
  }

  /// Put a handler to the pool.
  void put_handler(service_handler_type* handler_ptr)
  {
    // Need lock in multiple thread model.
    boost::asio::detail::mutex::scoped_lock lock(mutex_);

  	// If the pool has exceed high water mark or been closed, delete this handler.
    if (closed_ || (service_handlers_.size() >= pool_high_watermark_))
    {
      delete handler_ptr;
      --handler_count_;
      return;
    }

  	// Push this handler into the pool.
  	push_handler(handler_ptr);
  }

  /// Get the number of active handlers.
  std::size_t  get_load(void)
  {
    // Need lock in multiple thread model.
    boost::asio::detail::mutex::scoped_lock lock(mutex_);

    return (handler_count_ - service_handlers_.size());
  }

  /// Get the count of the handlers.
  std::size_t  handler_count(void)
  {
    // Need lock in multiple thread model.
    boost::asio::detail::mutex::scoped_lock lock(mutex_);

    return handler_count_;
  }

private:
  
  /// Get the allocator for work.
  work_allocator_type& work_allocator(void)
  {
    return *work_allocator_;
  }

  /// Release preallocated handler in the pool.
  void clear(void)
  {
    // Need lock in multiple thread model.
    boost::asio::detail::mutex::scoped_lock lock(mutex_);

    // Set flag
    closed_ = true;

    for (std::size_t i = 0; i < service_handlers_.size(); ++i)
      service_handlers_[i].reset();
    service_handlers_.clear();
  }
  
  /// Make a new handler.
  service_handler_type* make_handler(void)
  {
    return new service_handler_type(work_allocator().make_handler(),
        read_buffer_size_,
        write_buffer_size_,
        timeout_seconds_);
  }

  /// Push a handler into the pool.
  void push_handler(service_handler_type* handler_ptr)
  {
    service_handler_ptr service_handler(handler_ptr,
          bind(&service_handler_pool::put_handler,
              shared_from_this(),
              _1));
    service_handlers_.push_back(service_handler);
  }

  /// Create any handler to the pool.
  void create_handler(std::size_t count)
  {
    for (std::size_t i = 0; i < count; ++i)
    {
      push_handler(make_handler());
      ++handler_count_;
    }
  }

  /// Get a handler from the pool.
  service_handler_ptr get_handler(void)
  {
    // Need lock in multiple thread model.
    boost::asio::detail::mutex::scoped_lock lock(mutex_);

    // Add new handler if the pool is in low water mark.
    if (service_handlers_.size() <= pool_low_watermark_)
      create_handler(pool_increment_);

    service_handler_ptr service_handler = service_handlers_.back();
    service_handlers_.pop_back();
    return service_handler;
  }

private:
  /// Mutex to protect access to internal data.
  boost::asio::detail::mutex mutex_;

  /// Count of service_handler.
  std::size_t handler_count_;

  // Flag to indicate that the pool has been closed and all handlers need to be deleted.
  bool closed_;

  /// The pool of service_handler.
  std::vector<service_handler_ptr> service_handlers_;

  /// The allocator of work_handler.
  work_allocator_ptr work_allocator_;

  /// Preallocated handler number.
  std::size_t pool_init_size_;

  /// Low water mark of the pool.
  std::size_t pool_low_watermark_;

  /// High water mark of the pool.
  std::size_t pool_high_watermark_;

  /// Increase number of the pool.
  std::size_t pool_increment_;

  /// The maximum size for asynchronous read operation buffer.
  std::size_t read_buffer_size_;

  /// The maximum size for asynchronous write operation buffer.
  std::size_t write_buffer_size_;

  /// The expiry seconds of connection.
  std::size_t timeout_seconds_;
};

} // namespace bas

#endif // BAS_SERVICE_HANDLER_POOL_HPP
