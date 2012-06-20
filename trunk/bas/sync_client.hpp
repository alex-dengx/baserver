//
// sync_client.hpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2012 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BAS_SYNC_CLIENT_HPP
#define BAS_SYNC_CLIENT_HPP

#include <bas/io_service_pool.hpp>
#include <bas/sync_handler.hpp>

namespace bas {

#define BAS_SYNC_HANDLER_POOL_INIT_SIZE            10
#define BAS_SYNC_HANDLER_POOL_LOW_WATERMARK        0
#define BAS_SYNC_HANDLER_POOL_HIGH_WATERMARK       50
#define BAS_SYNC_HANDLER_POOL_INCREMENT            5
#define BAS_SYNC_HANDLER_POOL_MAXIMUM              500
#define BAS_SYNC_HANDLER_POOL_WAIT_MILLISECONDS    500

#define BAS_SYNC_HANDLER_BUFFER_DEFAULT_SIZE       256
#define BAS_SYNC_HANDLER_TIMEOUT_MILLISECONDS      30

/// Class for holding multi endpoint pair.
class endpoint_group
  : private boost::noncopyable
{
public:
  /// Define type reference of std::size_t.
  typedef std::size_t size_t;

  /// Define type reference of boost::recursive_mutex.
  typedef boost::recursive_mutex mutex_t;

  /// Define type reference of boost::recursive_mutex::scoped_lock.
  typedef boost::recursive_mutex::scoped_lock scoped_lock_t;

  /// Define type reference of boost::asio::ip::tcp::endpoint.
  typedef boost::asio::ip::tcp::endpoint endpoint_t;

  /// The type of value in the vector.
  typedef std::pair<endpoint_t, endpoint_t> endpoint_pair_t;

  /// Constructor.
  endpoint_group()
    : endpoint_pairs_(),
      mutex_(),
      next_endpoint_(0)
  {
  }

  /// Destructor.
  ~endpoint_group()
  {
    // Release all endpoint_pair.
    endpoint_pairs_.clear();
  }

  /// Set endpoint_pair.
  endpoint_group& set(endpoint_t& peer_endpoint,
                      endpoint_t& local_endpoint = endpoint_t())
  {
    // Lock for synchronize access to data.
    scoped_lock_t lock(mutex_);

    endpoint_pairs_.push_back(endpoint_pair_t(peer_endpoint, local_endpoint));

    return *this;
  }

  /// Return the size of endpoint_pairs.
  size_t size()
  {
    // Lock for synchronize access to data.
    scoped_lock_t lock(mutex_);
    
    return endpoint_pairs_.size();
  }

  /// Get one endpoint_pair_t to use.
  endpoint_pair_t get_endpoints()
  {
    // Lock for synchronize access to data.
    scoped_lock_t lock(mutex_);

    if (endpoint_pairs_.size() == 0)
      return endpoint_pair_t(endpoint_t(), endpoint_t());

    // Use a round-robin scheme to choose the next endpoint to use.
    if (next_endpoint_ >= endpoint_pairs_.size())
      next_endpoint_ = 0;

    return endpoint_pairs_[next_endpoint_++];
  }

  /// Get one endpoint_pair_t to use.
  endpoint_pair_t get_endpoints(size_t index)
  {
    // Lock for synchronize access to data.
    scoped_lock_t lock(mutex_);

    if (index >= endpoint_pairs_.size())
      return endpoint_pair_t(endpoint_t(), endpoint_t());
    else
      return endpoint_pairs_[index];
  }

private:
  /// Mutex for synchronize access to data.
  mutex_t mutex_;

  /// The next endpoint to use for a connection.
  size_t next_endpoint_;

  /// The endpoint_pair_t objects.
  std::vector<endpoint_pair_t> endpoint_pairs_;
};

/// A pool of sync_handler objects.
template<typename Socket_Service = boost::asio::ip::tcp::socket>
class sync_handler_pool
  : public boost::enable_shared_from_this<sync_handler_pool<Socket_Service> >,
    private boost::noncopyable
{
public:
  using boost::enable_shared_from_this<sync_handler_pool<Socket_Service> >::shared_from_this;

  /// Define type reference of std::size_t.
  typedef std::size_t size_t;

  /// The type of the socket that will be used to provide asynchronous operations.
  typedef Socket_Service socket_t;

  /// Define type reference of boost::recursive_mutex.
  typedef boost::recursive_mutex mutex_t;

  /// Define type reference of boost::recursive_mutex::scoped_lock.
  typedef boost::recursive_mutex::scoped_lock scoped_lock_t;

  /// The type of the sync_handler.
  typedef sync_handler<socket_t> sync_handler_t;
  typedef boost::shared_ptr<sync_handler_t> sync_handler_ptr;

  /// The type of the sync_handler_pool.
  typedef sync_handler_pool<socket_t> sync_handler_pool_t;
  typedef boost::shared_ptr<sync_handler_pool_t> sync_handler_pool_ptr;

  /// The type of the io_service_pool.
  typedef boost::shared_ptr<io_service_pool> io_service_pool_ptr;

  /// Constructor.
  sync_handler_pool(io_service_pool_ptr& io_pool,
      endpoint_group& endpoint_pairs,
      size_t buffer_size          = BAS_SYNC_HANDLER_BUFFER_DEFAULT_SIZE,
      long   timeout_milliseconds = BAS_SYNC_HANDLER_TIMEOUT_MILLISECONDS,
      size_t pool_init_size       = BAS_SYNC_HANDLER_POOL_INIT_SIZE,
      size_t pool_low_watermark   = BAS_SYNC_HANDLER_POOL_LOW_WATERMARK,
      size_t pool_high_watermark  = BAS_SYNC_HANDLER_POOL_HIGH_WATERMARK,
      size_t pool_increment       = BAS_SYNC_HANDLER_POOL_INCREMENT,
      size_t pool_maximum         = BAS_SYNC_HANDLER_POOL_MAXIMUM,
      long   wait_milliseconds    = BAS_SYNC_HANDLER_POOL_WAIT_MILLISECONDS)
    : io_pool_(io_pool),
      endpoint_pairs_(endpoint_pairs),
      buffer_size_(buffer_size),
      timeout_milliseconds_(timeout_milliseconds),
      pool_init_size_(pool_init_size),
      pool_low_watermark_(pool_low_watermark),
      pool_high_watermark_(pool_high_watermark),
      pool_increment_(pool_increment),
      pool_maximum_(pool_maximum),
      wait_milliseconds_(wait_milliseconds),
      mutex_(),
      condition_(),
      sync_handlers_(),
      handler_count_(0),
      closed_(true)
  {
    BOOST_ASSERT(io_pool_.get() != 0);

    BOOST_ASSERT(timeout_milliseconds_ != 0);
    BOOST_ASSERT(pool_init_size_ != 0);
    BOOST_ASSERT(pool_low_watermark_ <= pool_init_size_);
    BOOST_ASSERT(pool_high_watermark_ > pool_low_watermark_);
    BOOST_ASSERT(pool_maximum_ > pool_high_watermark_);
    BOOST_ASSERT(pool_increment_ != 0);
  }

  /// Create preallocated handlers to the pool.
  ///   Note: shared_from_this() can't be used in the constructor.
  void init(void)
  {
    // Lock for synchronize access to data.
    scoped_lock_t lock(mutex_);

    closed_ = false;

    // Create preallocated handlers to the pool.
    create_handler(pool_init_size_);
  }

  /// Release all handlers in the pool.
  ///   Note: close() can't be used in the destructor.
  void close(void)
  {
    // Release all handlers in the pool.
    clear();
  }

  /// Get an sync_handler to use.
  sync_handler_ptr get_sync_handler()
  {
    sync_handler_ptr sync_handler;
    boost::system_time const timeout = boost::get_system_time() + boost::posix_time::milliseconds(wait_milliseconds_);

    // Lock for synchronize access to data.
    scoped_lock_t lock(mutex_);

    for ( ; boost::get_system_time() <=  timeout; )
    {
      if (closed_)
        break;

      // Add new handler if the pool is in low water mark and handler count not exceed maximum.
      if (sync_handlers_.size() <= pool_low_watermark_ && handler_count_ < pool_maximum_)
        create_handler(pool_increment_);

      if (sync_handlers_.size() > 0)
      {
        sync_handler = sync_handlers_.back();
        sync_handlers_.pop_back();
      }

      if (sync_handler.get () != 0 || wait_milliseconds_ == 0)
        break;

      // Abort when timeout.
      if (!condition_.timed_wait(lock, timeout))
        break;
    }

    return sync_handler;
  }

  /// Put a handler to the pool.
  void put_handler(sync_handler_t* handler_ptr)
  {
    BOOST_ASSERT(handler_ptr != 0);

    // Lock for synchronize access to data.
    scoped_lock_t lock(mutex_);

    // Push this handler into the pool.
    if (!push_handler(handler_ptr))
      --handler_count_;
  }

  /// Get the count of the handlers.
  size_t handler_count(void)
  {
    // Lock for synchronize access to data.
    scoped_lock_t lock(mutex_);

    return handler_count_;
  }

private:
  /// Release handlers in the pool.
  void clear(void)
  {
    // Lock for synchronize access to data.
    scoped_lock_t lock(mutex_);

    if (closed_)
      return;

    closed_ = true;

    for (size_t i = 0; i < sync_handlers_.size(); ++i)
      sync_handlers_[i].reset();

    sync_handlers_.clear();
  }
  
  /// Make a new handler.
  sync_handler_t* make_handler(void)
  {
    endpoint_group::endpoint_pair_t endpoint_pair = endpoint_pairs_.get_endpoints();

    return new sync_handler_t(io_pool_->get_io_service(),
                   endpoint_pair.first,
                   endpoint_pair.second,
                   buffer_size_,
                   timeout_milliseconds_);
  }

  /// Push a handler into the pool.
  bool push_handler(sync_handler_t* handler_ptr)
  {
    if (handler_ptr == 0)
      return false;

    boost::system::error_code ec = handler_ptr->error_code();
    // If the pool has exceed high_water_mark or been closed, delete this handler.
    if (closed_                                   || \
        ec && ec != boost::asio::error::shut_down || \
        sync_handlers_.size() >= pool_high_watermark_)
    {
      handler_ptr->clear();
      delete handler_ptr;

      return false;
    }

    sync_handler_ptr sync_handler(handler_ptr,
          bind(&sync_handler_pool::put_handler,
              shared_from_this(),
              _1));

    sync_handlers_.push_back(sync_handler);

    // Notify one waited thread to wake up.
    condition_.notify_one();

    return true;
  }

  /// Create handlers to the pool.
  void create_handler(size_t count)
  {
    for (size_t i = 0; i < count; ++i)
      if (push_handler(make_handler()))
        ++handler_count_;
  }
 
private:
  /// The io_service_pool objects used to perform asynchronous operations.
  io_service_pool_ptr io_pool_;

  /// The endpoint_group objects holding multi endpoint pairs.
  endpoint_group& endpoint_pairs_;

  /// Mutex for synchronize access to data.
  mutex_t mutex_;

  /// Condition for notify changing.
  boost::condition condition_;

  /// Count of sync_handler.
  size_t handler_count_;

  // Flag to indicate that the pool has been closed and all handlers need to be deleted.
  bool closed_;

  /// The amount of milliseconds for condition timed_wait.
  long wait_milliseconds_;

  /// The pool of sync_handler.
  std::vector<sync_handler_ptr> sync_handlers_;

  /// Preallocated handler number.
  size_t pool_init_size_;

  /// Low water mark of the pool.
  size_t pool_low_watermark_;

  /// High water mark of the pool.
  size_t pool_high_watermark_;

  /// Increase number of the pool.
  size_t pool_increment_;

  /// Maximum number of the pool.
  size_t pool_maximum_;

  /// The size for asynchronous operation buffer.
  size_t buffer_size_;

  /// The timeout value in milliseconds.
  long timeout_milliseconds_;
};

/// The class of the sync_client.
template<typename Socket_Service = boost::asio::ip::tcp::socket>
class sync_client
  : private boost::noncopyable
{
public:
  /// Define type reference of std::size_t.
  typedef std::size_t size_t;

  /// The type of the socket that will be used to provide asynchronous operations.
  typedef Socket_Service socket_t;

  /// Define type reference of boost::asio::ip::tcp::endpoint.
  typedef boost::asio::ip::tcp::endpoint endpoint_t;

  /// The type of the sync_handler.
  typedef sync_handler<socket_t> sync_handler_t;
  typedef boost::shared_ptr<sync_handler_t> sync_handler_ptr;

  /// The type of the sync_handler_pool.
  typedef sync_handler_pool<socket_t> sync_handler_pool_t;
  typedef boost::shared_ptr<sync_handler_pool_t> sync_handler_pool_ptr;

  /// The type of the io_service_pool.
  typedef boost::shared_ptr<io_service_pool> io_service_pool_ptr;

  /// Constructor.
  sync_client(io_service_pool_ptr& io_pool,
      endpoint_group& endpoint_pairs,
      size_t buffer_size          = BAS_SYNC_HANDLER_BUFFER_DEFAULT_SIZE,
      long   timeout_milliseconds = BAS_SYNC_HANDLER_TIMEOUT_MILLISECONDS,
      size_t pool_init_size       = BAS_SYNC_HANDLER_POOL_INIT_SIZE,
      size_t pool_low_watermark   = BAS_SYNC_HANDLER_POOL_LOW_WATERMARK,
      size_t pool_high_watermark  = BAS_SYNC_HANDLER_POOL_HIGH_WATERMARK,
      size_t pool_increment       = BAS_SYNC_HANDLER_POOL_INCREMENT,
      size_t pool_maximum         = BAS_SYNC_HANDLER_POOL_MAXIMUM,
      long wait_milliseconds      = BAS_SYNC_HANDLER_POOL_WAIT_MILLISECONDS)
    : io_pool_(),
      sync_handler_pool_()
  {
    sync_handler_pool_.reset(new sync_handler_pool_t(io_pool,
                                     endpoint_pairs,
                                     buffer_size,
                                     timeout_milliseconds,
                                     io_pool_size,
                                     pool_init_size,
                                     pool_low_watermark,
                                     pool_high_watermark,
                                     pool_increment,
                                     pool_maximum,
                                     wait_milliseconds));

    // Start sync_client.
    start();
  }

  /// Constructor.
  sync_client(size_t io_pool_size,
      endpoint_group& endpoint_pairs,
      size_t buffer_size          = BAS_SYNC_HANDLER_BUFFER_DEFAULT_SIZE,
      long   timeout_milliseconds = BAS_SYNC_HANDLER_TIMEOUT_MILLISECONDS,
      size_t pool_init_size       = BAS_SYNC_HANDLER_POOL_INIT_SIZE,
      size_t pool_low_watermark   = BAS_SYNC_HANDLER_POOL_LOW_WATERMARK,
      size_t pool_high_watermark  = BAS_SYNC_HANDLER_POOL_HIGH_WATERMARK,
      size_t pool_increment       = BAS_SYNC_HANDLER_POOL_INCREMENT,
      size_t pool_maximum         = BAS_SYNC_HANDLER_POOL_MAXIMUM,
      long wait_milliseconds      = BAS_SYNC_HANDLER_POOL_WAIT_MILLISECONDS)
    : io_pool_(new io_service_pool(io_pool_size, io_pool_size)),
      sync_handler_pool_()
  {
    sync_handler_pool_.reset(new sync_handler_pool_t(io_pool_,
                                     endpoint_pairs,
                                     buffer_size,
                                     timeout_milliseconds,
                                     pool_init_size,
                                     pool_low_watermark,
                                     pool_high_watermark,
                                     pool_increment,
                                     pool_maximum,
                                     wait_milliseconds));

    // Start sync_client.
    start();
  }

  /// Constructor.
  sync_client(sync_handler_pool_t* sync_handler_pool)
    : io_pool_(),
      sync_handler_pool_(sync_handler_pool)
  {
    BOOST_ASSERT(sync_handler_pool_.get() != 0);

    // Start sync_client.
    start();
  }

  /// Destructor.
  ~sync_client()
  {
    // Stop sync_client.
    stop();

    // Destroy sync_handler pool.
    sync_handler_pool_.reset();

    // Destroy io_service pool.
    io_pool_.reset();
  }

  /// Start sync_client with non-blocked model.
  void start()
  {
    if (io_pool_.get() != 0)
      io_pool_->start();

    // Create preallocated handlers of the pool.
    sync_handler_pool_->init();
  }

  /// Stop sync_client.
  void stop()
  {
    sync_handler_pool_->close();

    if (io_pool_.get() != 0)
      io_pool_->stop();
  }

  /// Get an sync_handler to use.
  sync_handler_ptr get_sync_handler()
  {
    return sync_handler_pool_->get_sync_handler();
  }

private:
  /// The pool of sync_handler objects.
  sync_handler_pool_ptr sync_handler_pool_;

  /// The io_service_pool objects used to perform asynchronous operations.
  io_service_pool_ptr io_pool_;
};

} // namespace bas

#endif // BAS_SYNC_CLIENT_HPP
