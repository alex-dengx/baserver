//
// connections.hpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2009 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BAS_ECHO_CLIENT_CONNECTIONS_HPP
#define BAS_ECHO_CLIENT_CONNECTIONS_HPP

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <bas/io_service_pool.hpp>
#include <bas/service_handler_pool.hpp>
#include <bas/client.hpp>

#include <iostream>

#include <ctime>

#include "client_work.hpp"
#include "client_work_allocator.hpp"
#include "error_count.hpp"

namespace echo {

class connections
{
public:
  typedef bas::service_handler_pool<client_work, client_work_allocator> client_handler_pool_type;
  typedef bas::client<client_work, client_work_allocator> client_type;

  explicit connections(client_handler_pool_type* service_handler_pool,
      error_count* counter,
      const std::string& address,
      unsigned short port,
      std::size_t io_pool_size,
      std::size_t work_pool_init_size,
      std::size_t work_pool_high_watermark,
      unsigned int pause_seconds,
      std::size_t connection_number = 1000,
      unsigned int wait_seconds = 10,
      unsigned int test_times = 10)
    : io_service_pool_(io_pool_size),
      work_service_pool_(work_pool_init_size, work_pool_high_watermark),
      client_(service_handler_pool, address, port),
      error_count_(counter),
      io_service_(),
      timer_(io_service_),
      time_long_(boost::posix_time::milliseconds(0)),
      pause_seconds_(pause_seconds),
      connection_number_(connection_number),
      wait_seconds_(wait_seconds),
      test_times_(test_times)
  {
    BOOST_ASSERT(connection_number != 0);
    BOOST_ASSERT(wait_seconds != 0);
    BOOST_ASSERT(test_times != 0);
  }

  void run(void)
  {
    run(false);
  }

  void run(bool force_stop)
  {
    std::cout << "Start test for " << test_times_ << " times and once with " << connection_number_ << " connections.\n";
    if (pause_seconds_ != 0)
      std::cout << "for test server concurrent processing, client pause " << pause_seconds_ << " seconds before send data.\n";
    std::cout << "\n";

    std::size_t counts = 0;
    
    for (std::size_t i = 0; i < test_times_; ++i)
    {
      run_once(force_stop);
      counts += connection_number_;

      std::cout << "Established connections " << counts << ", time " << time_long_.total_milliseconds() << " ms. ";
      std::cout << "average " << time_long_.total_milliseconds()/(i+1) << " ms.\n";
      if (error_count_->get_timeout() != 0)
      {
        std::cout << "Total " << error_count_->get_timeout() << " connections timeout. ";
        std::cout << "everage " << error_count_->get_timeout()/(i+1) << " timeout.\n";
      }
      if (error_count_->get_error() != 0)
      {
        std::cout << "Total " << error_count_->get_error() << " connections failed. ";
        std::cout << "everage " << error_count_->get_error()/(i+1) << " failed.\n";
      }

      if (i < test_times_ - 1)
      {
        std::cout << "Wait " << wait_seconds_ << " seconds for next test. remain " << test_times_ - i - 1 <<" times.\n\n";
        timer_.expires_from_now(boost::posix_time::seconds(wait_seconds_));
        timer_.wait();
      }
      else
        std::cout << "\n";
    }
    
    std::cout << "All test done! total established connections " << counts << ", time " << time_long_.total_milliseconds() << " ms.";
    if (error_count_->get_timeout() != 0)
      std::cout << " total " << error_count_->get_timeout() << " connections timeout.";
    if (error_count_->get_error() != 0)
      std::cout << " total " << error_count_->get_error() << " connections failed.";
    std::cout << "\n";
  }

  void run_once(bool force_stop)
  {
    boost::posix_time::ptime time_start;
    boost::posix_time::time_duration time_long;

    std::size_t error_timeout = error_count_->get_timeout();
    std::size_t error_other = error_count_->get_error();

    std::cout << "Creating " << connection_number_ << " connections.\n";
    time_start = boost::posix_time::microsec_clock::universal_time();

    work_service_pool_.start();
    io_service_pool_.start();

    for (std::size_t i = 0; i < connection_number_; ++i)
      client_.connect(io_service_pool_.get_io_service(), work_service_pool_.get_io_service());

    time_long = boost::posix_time::microsec_clock::universal_time() - time_start;
    std::cout << "All connections created in " << time_long.total_milliseconds() << " ms.\n";
      
    if (force_stop)
    {
      io_service_pool_.stop(force_stop);
      work_service_pool_.stop(force_stop);
    }
    else
    {
      io_service_pool_.stop();
      work_service_pool_.stop();

      while (!io_service_pool_.is_free() || !work_service_pool_.is_free())
      {
        work_service_pool_.start();
        io_service_pool_.start();
        io_service_pool_.stop();
        work_service_pool_.stop();
      }
    }

    error_timeout = error_count_->get_timeout() - error_timeout;
    error_other = error_count_->get_error() - error_other;

    time_long = boost::posix_time::microsec_clock::universal_time() - time_start;
    std::cout << "All connections complete in " << time_long.total_milliseconds() << " ms.";
    if (error_timeout != 0)
      std::cout << " total " << error_timeout << " connections timeout.";
    if (error_other != 0)
      std::cout << " total " << error_other << " connections failed.";
    std::cout << "\n";
    
    time_long_ += time_long;
  }

private:
  bas::io_service_pool io_service_pool_;
  bas::io_service_pool work_service_pool_;
  client_type client_;
  error_count* error_count_;
  
  boost::asio::io_service io_service_;
  boost::asio::deadline_timer timer_;
  boost::posix_time::time_duration time_long_;
  std::size_t connection_number_;
  unsigned int wait_seconds_;
  unsigned int test_times_;
  unsigned int pause_seconds_;
};

} // namespace echo

#endif // BAS_ECHO_CLIENT_CONNECTIONS_HPP
