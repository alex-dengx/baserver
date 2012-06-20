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
#include <bas/sync_client.hpp>
#include <iostream>
#include <ctime>

#include "error_count.hpp"

namespace echo {

std::string echo_message = "echo server test message.....\r\n";
std::size_t mlen = echo_message.length();
unsigned char *msg = (unsigned char *)echo_message.c_str();

class connections
{
public:
  typedef bas::sync_handler<boost::asio::ip::tcp::socket> sync_handler_t;
  typedef bas::sync_client<boost::asio::ip::tcp::socket> client_t;
  typedef boost::shared_ptr<sync_handler_t> sync_handler_ptr;

  connections(std::size_t io_pool_size,
      bas::endpoint_group& endpoint_pairs,
      std::size_t buffer_size,
      long timeout_milliseconds,
      std::size_t preallocated_handler_number,
      error_count& counter,
      unsigned int pause_seconds,
      std::size_t connection_number = 1000,
      unsigned int wait_seconds = 10,
      unsigned int test_times = 10)
    : client_(io_pool_size,
              endpoint_pairs,
              buffer_size,
              timeout_milliseconds,
              preallocated_handler_number),
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

  void run()
  {
    std::cout << "Start test for " << test_times_ << " times and once with " << connection_number_ << " connections.\n";
    if (pause_seconds_ != 0)
      std::cout << "for test server concurrent processing, client pause " << pause_seconds_ << " seconds before send data.\n";
    std::cout << "\n";

    std::size_t counts = 0;
    
    for (std::size_t i = 0; i < test_times_; ++i)
    {
      run_once();
      counts += connection_number_;

      std::cout << "Established connections " << counts << ", time " << time_long_.total_milliseconds() << " ms. ";
      std::cout << "average " << time_long_.total_milliseconds()/(i+1) << " ms.\n";
      if (error_count_.get_timeout() != 0)
      {
        std::cout << "Total " << error_count_.get_timeout() << " connections timeout. ";
        std::cout << "everage " << error_count_.get_timeout()/(i+1) << " timeout.\n";
      }
      if (error_count_.get_error() != 0)
      {
        std::cout << "Total " << error_count_.get_error() << " connections failed. ";
        std::cout << "everage " << error_count_.get_error()/(i+1) << " failed.\n";
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
    if (error_count_.get_timeout() != 0)
      std::cout << " total " << error_count_.get_timeout() << " connections timeout.";
    if (error_count_.get_error() != 0)
      std::cout << " total " << error_count_.get_error() << " connections failed.";
    std::cout << "\n";
  }

  void run_once()
  {
    boost::posix_time::ptime time_start;
    boost::posix_time::time_duration time_long;

    std::size_t error_timeout = error_count_.get_timeout();
    std::size_t error_other = error_count_.get_error();

    std::size_t bytes_transferred = 0;
    boost::system::error_code  ec;

    std::cout << "Creating " << connection_number_ << " connections.\n";
    time_start = boost::posix_time::microsec_clock::universal_time();

    for (std::size_t i = 0; i < connection_number_; ++i)
    {
      bytes_transferred = 0;
      sync_handler_ptr sync_handler = client_.get_sync_handler();
      if (sync_handler.get() == 0)
      { 
        std::cout << "Get handler error: not free handler.\n";
        error_count_.error();
        continue;
      }
 
      if (ec = sync_handler->connect())
      {
        std::cout << "Connect error: " << ec.message() << "\n";
        error_count_.error();
        continue;
      }

      sync_handler->buffer().clear();
      sync_handler->buffer().produce(mlen, msg);
/*
      if (ec = sync_handler->write(bytes_transferred))
      {
        std::cout << "Write error: " << ec.message() << "\n";
        error_count_.error();
        continue;
      }

      sync_handler->buffer().clear();
      if (ec = sync_handler->read_some(bytes_transferred))
      {
        std::cout << "Read error: " << ec.message() << "\n";
        error_count_.error();
        continue;
      }
*/
      if (ec = sync_handler->write_read(bytes_transferred))
      {
        std::cout << "Write and read error: " << ec.message() << "\n";
        error_count_.error();
        continue;
      }

      if (mlen != bytes_transferred || memcmp(msg, sync_handler->buffer().data(), mlen) != 0)
      {
        std::cout << "Read error: received message is unexpected.\n";
        error_count_.error();
      }
    }

    error_timeout = error_count_.get_timeout() - error_timeout;
    error_other = error_count_.get_error() - error_other;

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
  client_t client_;
  boost::asio::io_service io_service_;
  boost::asio::ip::tcp::endpoint endpoint_;
  boost::asio::deadline_timer timer_;
  boost::posix_time::time_duration time_long_;

  error_count& error_count_;

  std::size_t connection_number_;
  unsigned int wait_seconds_;
  unsigned int test_times_;
  unsigned int pause_seconds_;
};

} // namespace echo

#endif // BAS_ECHO_CLIENT_CONNECTIONS_HPP
