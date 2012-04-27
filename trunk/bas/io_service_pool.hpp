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

  /// Construct the io_service pool.
  explicit io_service_pool(size_type pool_init_size = BAS_IO_SERVICE_POOL_INIT_SIZE,
      size_type pool_high_watermark = BAS_IO_SERVICE_POOL_HIGH_WATERMARK,
      size_type pool_thread_load = BAS_IO_SERVICE_POOL_THREAD_LOAD)
    : mutex_(),
      io_services_(),
      work_(),
      threads_(),
      pool_high_watermark_(pool_high_watermark),
      pool_thread_load_(pool_thread_load),
      next_io_service_(0),
      is_free_(true),
      block_(false)
  {
    BOOST_ASSERT(pool_init_size != 0);
    BOOST_ASSERT(pool_high_watermark >= pool_init_size);
    BOOST_ASSERT(pool_thread_load != 0);

    // Create io_service pool.
    for (size_type i = 0; i < pool_init_size; ++i)
    {
      io_service_ptr io_service(new boost::asio::io_service);
      io_services_.push_back(io_service);
    }
  }
  
  /// Destruct the pool object.
  ~io_service_pool()
  {
    // Stop all io_service objects in the pool.
    stop();

    // Destroy io_service pool.
    for (size_type i = 0; i < io_services_.size(); ++i)
      io_services_[i].reset();

    io_services_.clear();
  }

  /// Get the size of the pool.
  size_type size() const
  {
    return io_services_.size();
  }

  /// Get the load of each thread.
  size_type get_thread_load() const
  {
    return pool_thread_load_;
  }

  /// Get work status of the pool.
  bool is_free(void)
  {
    // Need lock in multiple thread model.
    boost::asio::detail::mutex::scoped_lock lock(mutex_);

    return is_free_;
  }

  /// Start all io_service objects in nonblock model.
  void start()
  {
    start(false);
  }

  /// Run all io_service objects in block model.
  void run()
  {
    start(true);
  }

  /// Stop all io_service objects in the pool with gracefully mode,
  ///   All work will be finished and there are no more handlers to be dispatched.
  void stop()
  {
    stop(false);
  }

  /// Stop all io_service objects in the pool.
  void stop(bool force)
  {
    // Allow all operations and handlers to be finished normally,
    //   the work object may be explicitly destroyed.

    // Destroy all work.
    for (size_type i = 0; i < work_.size(); ++i)
      work_[i].reset();

    work_.clear();

    // If in force mode, maybe some handlers cannot be dispatched.
    if (force)
      force_stop();

    // If in block mode, wait for all threads in the pool to exit.
    if (!block_)
      wait();
  }

  /// Get an io_service to use.
  boost::asio::io_service& get_io_service()
  {
    // Use a round-robin scheme to choose the next io_service to use.
    boost::asio::io_service& io_service = *io_services_[next_io_service_];
    if (++next_io_service_ == io_services_.size())
      next_io_service_ = 0;

    return io_service;
  }

  /// Get an io_service to use. if need then create one to use.
  boost::asio::io_service& get_io_service(size_type load)
  {
    // Calculate the required number of threads.
    size_type threads_number = load / pool_thread_load_;
    if ((threads_number > io_services_.size()) && (io_services_.size() < pool_high_watermark_) && !block_)
    {
      // Create new io_service and start it.
      io_service_ptr io_service(new boost::asio::io_service);
      io_services_.push_back(io_service);
      start_one(io_service);
      next_io_service_ = io_services_.size() - 1;
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
    if (threads_.size() == 0)
      return;

    // Wait for all threads in the pool to exit.
    for (size_type i = 0; i < threads_.size(); ++i)
      threads_[i]->join();

    // Destroy all threads.
    threads_.clear();
  }

  /// Run an io_service.
  void run_service(io_service_ptr io_service)
  {
    // Run the io_service and check executed handler number is zero or not.
    if ((io_service->run() != 0) && is_free_)
    {
      // Need lock in multiple thread model.
      boost::asio::detail::mutex::scoped_lock lock(mutex_);

      // Some handlers has been executed, set to false.
      is_free_ = false;
    }
  }

  /// Start an io_service.
  void start_one(io_service_ptr io_service)
  {
    // Reset the io_service in preparation for a subsequent run() invocation.
    io_service->reset();

    // Give the io_service work to do so that its run() functions will not
    //   exit until work was explicitly destroyed.
    work_ptr work(new boost::asio::io_service::work(*io_service));
    work_.push_back(work);

    // Create a thread to run the io_service.
    thread_ptr thread(new boost::thread(
        boost::bind(&io_service_pool::run_service, this, io_service)));

    threads_.push_back(thread);
  }

  /// Start all io_service objects in the pool.
  void start(bool block)
  {
    if (threads_.size() != 0)
      return;

    // The pool has not do anything now, reset to true.
    is_free_ = true;

    // Start all io_service.
    for (size_type i = 0; i < io_services_.size(); ++i)
      start_one(io_services_[i]);

    block_ = block;

    // If in block mode, wait for all threads in the pool to exit.
    if (block)
      wait();
  }

  /// Force stop all io_service objects in the pool.
  void force_stop(void)
  {
    // Force stop all io_service, maybe some handlers cannot be dispatched.
    for (size_type i = 0; i < io_services_.size(); ++i)
      io_services_[i]->stop();
  }

private:
  /// Mutex to protect access to internal data.
  boost::asio::detail::mutex mutex_;

  /// Work status of the pool.
  bool is_free_;

  /// The pool of io_services.
  std::vector<io_service_ptr> io_services_;

  /// The work that keeps the io_services running.
  std::vector<work_ptr> work_;

  /// The pool of threads for running individual io_service.
  std::vector<thread_ptr> threads_;

  /// High water mark of the pool.
  size_type pool_high_watermark_;

  /// Carrying the load of each thread.
  size_type pool_thread_load_;

  /// The next io_service to use for a connection.
  size_type next_io_service_;

  /// Flag to indicate that start() functions will block or not.
  bool block_;
};

} // namespace bas

#endif // BAS_IO_SERVICE_POOL_HPP
