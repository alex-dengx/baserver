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

#include <bas/service_handler.hpp>

#include <iostream>

namespace echo {

std::string echo_message = "echo server test message.\r\n";
std::size_t mlen = echo_message.length();
char *msg = (char *)echo_message.c_str();

class client_work
{
public:
  typedef bas::service_handler<client_work> client_handler_type;

  client_work()
  {
  }

  void on_clear(client_handler_type& handler)
  {
  }

  void on_open(client_handler_type& handler)
  {
/*
    boost::posix_time::ptime time_epoch(boost::gregorian::date(1970,1,1)); 
    boost::posix_time::ptime time_start = boost::posix_time::microsec_clock::universal_time();
    boost::posix_time::time_duration td_epoch = time_start - time_epoch;
    srand(td_epoch.total_milliseconds());

    handler.async_write(boost::asio::buffer(echo_message, rand() % echo_message.size() + 1));
*/
    handler.async_write(boost::asio::buffer(echo_message));
  }

  void on_read(client_handler_type& handler, std::size_t bytes_transferred)
  {
    if (mlen != bytes_transferred || memcmp(handler.read_buffer().data(), msg, mlen) != 0)
    {
      std::cout << "Receive data error.\n";
      std::cout.flush();
    }
    handler.close();
  }

  void on_write(client_handler_type& handler, std::size_t bytes_transferred)
  {
    handler.async_read_some();
  }

  void on_close(client_handler_type& handler, const boost::system::error_code& e)
  {
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
        std::cout << "client error " << e << " message " << e.message() << "\n";
        std::cout.flush();
        break;

      // Connection breaked.
      case boost::asio::error::connection_aborted:
      case boost::asio::error::connection_reset:
      case boost::asio::error::connection_refused:
        break;

      // Other error.
      case boost::asio::error::no_buffer_space:
        std::cout << "client error " << e << " message " << e.message() << "\n";
        std::cout.flush();
        break;
    }
  }

  void on_parent(client_handler_type& handler, const bas::event event)
  {
  }

  void on_child(client_handler_type& handler, const bas::event event)
  {
  }
};

} // namespace echo

#endif // BAS_ECHO_CLIENT_WORK_HPP
