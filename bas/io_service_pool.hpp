//
// io_service_pool.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2009, 2011 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BAS_IO_SERVICE_POOL_HPP
#define BAS_IO_SERVICE_POOL_HPP

#include <boost/assert.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/asio/detail/mutex.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>

namespace bas {

#define BAS_IO_SERVICE_POOL_INIT_SIZE       4
#define BAS_IO_SERVICE_POOL_HIGH_WATERMARK  32
#define BAS_IO_SERVICE_POOL_THREAD_LOAD     100

/// A pool of io_service objects.
class io_service_pool
  : private boost::noncopyable
{
public:
  /// Define type reference of std::size_t.
  typedef std::size_t size_type;

  /// Define type reference of boost::asio::detail::mutex.
  typedef boost::asio::detail::mutex mutex_t;

  /// Define type reference of boost::asio::detail::mutex::scoped_lock.
  typedef boost::asio::detail::mutex::scoped_lock scoped_lock_t;

  /// Constructor.
  io_service_pool(size_t pool_init_size = BAS_IO_SERVICE_POOL_INIT_SIZE,
      size_t pool_high_watermark = BAS_IO_SERVICE_POOL_HIGH_WATERMARK,
      size_t pool_thread_load = BAS_IO_SERVICE_POOL_THREAD_LOAD)
    : mutex_(),
      io_services_(),
      threads_(),
      work_(),
      pool_init_size_(pool_init_size),
      pool_high_watermark_(pool_high_watermark),
      pool_thread_load_(pool_thread_load),
      next_io_service_(0),
      blocked_(false),
      idle_(true)
  {
    BOOST_ASSERT(pool_init_size_ != 0);
    BOOST_ASSERT(pool_high_watermark_ >= pool_init_size_);
    BOOST_ASSERT(pool_thread_load_ != 0);

    // Create io_service pool.
    for (size_t i = 0; i < pool_init_size_; ++i)
      io_services_.push_back(io_service_ptr(new boost::asio::io_service));
  }

  /// Destruct the pool object.
  ~io_service_pool()
  {
    // Stop all io_service objects in the pool.
    stop();

    // Destroy io_service pool.
    for (size_t i = io_services_.size(); i > 0 ; --i)
      io_services_[i - 1].reset();

    io_services_.clear();
  }

  /// Set pool parameters.
  io_service_pool& set(size_t pool_init_size = BAS_IO_SERVICE_POOL_INIT_SIZE,
      size_t pool_high_watermark = BAS_IO_SERVICE_POOL_HIGH_WATERMARK,
      size_t pool_thread_load = BAS_IO_SERVICE_POOL_THREAD_LOAD)
  {
    BOOST_ASSERT(pool_init_size != 0);
    BOOST_ASSERT(pool_high_watermark >= pool_init_size);
    BOOST_ASSERT(pool_thread_load != 0);

    if (threads_.empty())
    {
      pool_init_size_ = pool_init_size;
      pool_high_watermark_ = pool_high_watermark;
      pool_thread_load_ = pool_thread_load;
    }

    return *this;
  }

  /// Get the size of the pool.
  size_t size()
  {
    // Lock for synchronize access to data.
    scoped_lock_t lock(mutex_);

    return io_services_.size();
  }

  /// Get the load of each thread.
  size_t get_thread_load() const
  {
    return pool_thread_load_;
  }

  /// Get work status of the pool.
  bool idle()
  {
    // Lock for synchronize access to data.
    scoped_lock_t lock(mutex_);

    return idle_;
  }

  /// Run all io_service objects in blocked model.
  void run()
  {
    start(true);
  }

  /// Start all io_service objects, default with non-bocked mode.
  void start(bool blocked = false)
  {
    if (!threads_.empty())
      return;

    {
      // Lock for synchronize access to data.
      scoped_lock_t lock(mutex_);

      blocked_ = blocked;

      // Create additional io_service pool.
      for (size_t i = io_services_.size(); i < pool_init_size_; ++i)
        io_services_.push_back(io_service_ptr(new boost::asio::io_service));

      // Release redundant io_service pool.
      for (size_t i = io_services_.size(); i > pool_init_size_; --i)
        io_services_.pop_back();

      // The pool is still idle now, set to true.
      idle_ = true;

      // Start all io_service.
      for (size_t i = io_services_.size(); i > 0 ; --i)
        start_one(io_services_[i - 1]);
    }

    // If in block mode, wait for all threads to exit.
    if (blocked_)
      wait();
  }

  /// Stop all io_service objects, default with gracefully mode.
  void stop(bool force = false)
  {
    if (work_.empty())
      return;

    {
      // Lock for synchronize access to data.
      scoped_lock_t lock(mutex_);

      // Allow all operations and handlers to be finished normally,
      //   the work object may be explicitly destroyed.
      for (size_t i = work_.size(); i > 0 ; --i)
        work_[i - 1].reset();

      work_.clear();
    }

    // If in force mode, maybe some handlers cannot be dispatched.
    if (force)
      force_stop();

    // If in block mode, wait for all threads in the pool to exit.
    if (!blocked_)
      wait();
  }

  /// Get an io_service to use.
  boost::asio::io_service& get_io_service()
  {
    // Lock for synchronize access to data.
    scoped_lock_t lock(mutex_);

    // Use a round-robin scheme to choose the next io_service to use.
    if (next_io_service_ >= io_services_.size())
      next_io_service_ = 0;

    return *io_services_[next_io_service_++];
  }

  /// Get an io_service to use. if need then create one to use.
  boost::asio::io_service& get_io_service(size_t load)
  {
    // Calculate the required number of threads.
    size_t threads_number = load / pool_thread_load_;

    // Lock for synchronize access to data.
    scoped_lock_t lock(mutex_);

    size_t service_count = io_services_.size();
    if (!blocked_                            && \
        !work_.empty()                       && \
        !threads_.empty()                    && \
        threads_number > service_count       && \
        service_count < pool_high_watermark_)
    {
      // Create new io_service and start it.
      io_service_ptr io_service(new boost::asio::io_service);
      io_services_.push_back(io_service);
      start_one(io_service);
      next_io_service_ = service_count;
    }

    return get_io_service();
  }

private:
  typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;
  typedef boost::shared_ptr<boost::asio::io_service::work> work_ptr;
  typedef boost::shared_ptr<boost::thread> thread_ptr;

  /// Wait for all threads in the pool to exit.
  void wait()
  {
    if (threads_.empty())
      return;

    // Wait for all threads in the pool to exit.
    for (size_t i = threads_.size(); i > 0 ; --i)
      threads_[i - 1]->join();

    // Destroy all threads.
    threads_.clear();
  }

  /// Run an io_service.
  void run_service(io_service_ptr io_service)
  {
    // Run the io_service and check executed handler number.
    if (io_service->run() != 0)
    {
      // Lock for synchronize access to data.
      scoped_lock_t lock(mutex_);

      // Some handlers has been executed, set to false.
      idle_ = false;
    }
  }

  /// Start an io_service.
  void start_one(io_service_ptr io_service)
  {
    // Reset the io_service in preparation for a subsequent run() invocation.
    io_service->reset();

    // Give the io_service work to do so that its run() functions will not
    //   exit until work was explicitly destroyed.
    work_.push_back(work_ptr(new boost::asio::io_service::work(*io_service)));

    // Create a thread to run the io_service.
    threads_.push_back(thread_ptr(new boost::thread(boost::bind(&io_service_pool::run_service,
                                                                this,
                                                                io_service))));
  }

  /// Force stop all io_service objects in the pool.
  void force_stop(void)
  {
    // Force stop all io_service, maybe some handlers cannot be dispatched.
    for (size_t i = io_services_.size(); i > 0; --i)
      io_services_[i - 1]->stop();
  }

private:
  /// Mutex for synchronize access to data.
  mutex_t mutex_;

  /// Flag to indicate start mode.
  bool blocked_;

  /// Flag to indicate work status.
  bool idle_;

  /// The pool of io_services.
  std::vector<io_service_ptr> io_services_;

  /// The pool of threads for running individual io_service.
  std::vector<thread_ptr> threads_;

  /// The work that keeps the io_services running.
  std::vector<work_ptr> work_;

  /// Initialize size of the pool.
  size_t pool_init_size_;

  /// High water mark of the pool.
  size_t pool_high_watermark_;

  /// The carrying load of each thread.
  size_t pool_thread_load_;

  /// The next io_service to use for a connection.
  size_t next_io_service_;
};

} // namespace bas

#endif // BAS_IO_SERVICE_POOL_HPP
