//
// client_work.hpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2009 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BAS_ECHO_CLIENT_WORK_HPP
#define BAS_ECHO_CLIENT_WORK_HPP

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <bas/service_handler.hpp>
#include "error_count.hpp"

#include <iostream>

namespace echo {

std::string echo_message = "echo server test message.....\r\n";
std::size_t mlen = echo_message.length();
char *msg = (char *)echo_message.c_str();

class client_work
{
public:
  typedef bas::service_handler<client_work> client_handler_type;

  client_work(error_count* counter, unsigned int pause_time)
    : error_count_(counter),
      pause_time_(pause_time)
  {
  }

  void on_clear(client_handler_type& handler)
  {
  }

  /// Handle timeout of whole operation in io_service thread.
  void handle_timeout(boost::shared_ptr<client_handler_type> handler, const boost::system::error_code& e)
  {
    // The timer has been cancelled, do nothing.
    if (e == boost::asio::error::operation_aborted)
      return;

    if (timer_.get() != 0)
      handler->async_write(boost::asio::buffer(echo_message));
  }

  void on_open(client_handler_type& handler)
  {
    if (pause_time_ != 0)
    {
      timer_.reset(new boost::asio::deadline_timer(handler.io_service()));
      timer_->expires_from_now(boost::posix_time::seconds(pause_time_));
      timer_->async_wait(boost::bind(&client_work::handle_timeout,
          this,
          handler.shared_from_this(),
          boost::asio::placeholders::error));
    }
    else
      handler.async_write(boost::asio::buffer(echo_message));
  }

  void on_read(client_handler_type& handler, std::size_t bytes_transferred)
  {
    if (mlen != bytes_transferred || memcmp(handler.read_buffer().data(), msg, mlen) != 0)
    {
      error_count_->error();
      //std::cout << "Receive data error. length is [" << bytes_transferred << "] expect is [" << mlen << "]\n";
      //std::cout.flush();
    }

    handler.close();
  }

  void on_write(client_handler_type& handler, std::size_t bytes_transferred)
  {
    if (timer_.get() != 0)
      timer_.reset();

    handler.async_read_some();
  }

  void on_close(client_handler_type& handler, const boost::system::error_code& e)
  {
    if (timer_.get() != 0)
    {
      timer_->cancel();
      timer_.reset();
    }
    
    switch (e.value())
    {
      // Operation successfully completed.
      case 0:
      case boost::asio::error::eof:
        //std::cout << "connection is closed normally.\n";
        //std::cout.flush();
        break;

      // Operation timed out.
      case boost::asio::error::timed_out:
        error_count_->timeout();
        //std::cout << "client error " << e << " message " << e.message() << "\n";
        //std::cout.flush();
        break;

      // Connection breaked.
      case boost::asio::error::connection_aborted:
      case boost::asio::error::connection_reset:
      case boost::asio::error::connection_refused:
      default:
        error_count_->error();
        break;

      // Other error.
      case boost::asio::error::no_buffer_space:
        //std::cout << "client error " << e << " message " << e.message() << "\n";
        //std::cout.flush();
        break;
    }
  }

  void on_parent(client_handler_type& handler, const bas::event event)
  {
  }

  void on_child(client_handler_type& handler, const bas::event event)
  {
  }

private:

  typedef boost::shared_ptr<boost::asio::deadline_timer> timer_ptr;

  error_count* error_count_;

  unsigned int pause_time_;

  timer_ptr timer_;
};

} // namespace echo

#endif // BAS_ECHO_CLIENT_WORK_HPP
