//
// io_service_group.hpp
// ~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2009, 2012 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BAS_IO_SERVICE_GROUP_HPP
#define BAS_IO_SERVICE_GROUP_HPP

#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>
#include <bas/io_service_pool.hpp>
#include <vector>

namespace bas {

/// Class for holding multi io_service_pool.
class io_service_group
  : private boost::noncopyable
{
public:
  /// Define type reference of std::size_t.
  typedef std::size_t size_t;

  /// Define shared_ptr type of io_service_pool.
  typedef boost::shared_ptr<io_service_pool> io_service_pool_ptr;

  /// Define index used by server class.
  enum index_t
  {
    io_pool = 0,
    work_pool = 1
  };

  /// Constructor.
  io_service_group(size_t group_size = work_pool + 1,
      bool force_stop = false)
    : io_service_pools_(group_size, io_service_pool_ptr(new io_service_pool(1, 1))),
      force_stop_(force_stop),
      started_(false)
  {
    BOOST_ASSERT(group_size > work_pool);
  }

  /// Destructor.
  ~io_service_group()
  {
    // Stop all io_service_pool.
    stop();

    // Release all io_service_pool.
    clear();
  }

  /// Set stop mode to graceful or force.
  io_service_group& set(bool force_stop = false)
  {
    if (!started_)
      force_stop_ = force_stop;

    return *this;
  }

  /// Get specified io_service_pool to use.
  io_service_pool& get(size_t index)
  {
    BOOST_ASSERT(index < io_service_pools_.size());

    return *io_service_pools_[index];
  }

  /// Get started status of the io_service_group.
  bool started() const
  {
    return started_;
  }

  /// Start the io_service_group with non-blocked mode.
  void start()
  {
    if (started_)
      return;

    // Start all io_service_pool with non-blocked mode.
    for (size_t i = io_service_pools_.size(); i > 0; --i)
      io_service_pools_[i - 1]->start();

    started_ = true;
  }

  /// Stop the io_service_group.
  void stop()
  {
    if (!started_)
      return;

    // Stop all io_service_pool.
    for (size_t i = 0; i < io_service_pools_.size(); ++i)
      io_service_pools_[i]->stop(force_stop_);

    // For gracefully close, continue to repeat several times to dispatch and perform asynchronous operations/handlers.
    while (!force_stop_)
    {
      bool is_free = true;
      for (size_t i = 0; is_free && i < io_service_pools_.size(); ++i)
        is_free = io_service_pools_[i]->is_free();

      if (is_free)
        break;

      // Start all io_service_pool with non-blocked mode.
      for (size_t i = io_service_pools_.size(); i > 0; --i)
        io_service_pools_[i - 1]->start();

      // Stop all io_service_pool with graceful mode.
      for (size_t i = 0; i < io_service_pools_.size(); ++i)
        io_service_pools_[i]->stop();
    }

    started_ = false;
  }

private:
  /// Clear and release shared_pre of io_service_pool.
  void clear()
  {
    // Stop all io_service_pool.
    for (size_t i = 0; i < io_service_pools_.size(); ++i)
      io_service_pools_[i].reset();

    io_service_pools_.clear();
  }

private:
  /// The io_service_pool objects used to perform synchronous works.
  std::vector<io_service_pool_ptr> io_service_pools_;
  
  /// Start flag of the io_service_group.
  bool started_;

  /// Stop mode of the io_service_group.
  bool force_stop_;
};

} // namespace bas

#endif // BAS_IO_SERVICE_GROUP_HPP
