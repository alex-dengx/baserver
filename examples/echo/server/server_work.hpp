//
// server_work.hpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2009 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BAS_ECHO_SERVER_WORK_HPP
#define BAS_ECHO_SERVER_WORK_HPP

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <bas/service_handler.hpp>

#include <iostream>

namespace echo {

class server_work
{
public:

  typedef bas::service_handler<server_work> server_handler_type;

  server_work()
  {
  }
  
  void on_clear(server_handler_type& handler)
  {
  }
  
  void on_open(server_handler_type& handler)
  {
    //std::cout << "connect ok ...............\n";
    //std::cout.flush();

    //std::cout << "client ip is " << handler.socket().remote_endpoint().address().to_string() << "\n";
    //std::cout.flush();

    time_start = boost::posix_time::microsec_clock::universal_time();
    handler.async_read_some();
  }

  void on_read(server_handler_type& handler, std::size_t bytes_transferred)
  {
    //std::cout << "receive " << bytes_transferred << " bytes ok ...............\n";
    //std::cout.flush();

    handler.async_write(boost::asio::buffer(handler.read_buffer().data(), bytes_transferred));
  }

  void on_write(server_handler_type& handler, std::size_t bytes_transferred)
  {
    //std::cout << "send " << bytes_transferred << " bytes ok ...............\n";
    //std::cout.flush();

    handler.read_buffer().clear();
    handler.async_read_some();

//    handler.close();
  }

  void on_close(server_handler_type& handler, const boost::system::error_code& e)
  {
    switch (e.value())
    {
      // Operation successfully completed.
      case 0:
      case boost::asio::error::eof:
        //std::cout << "close ok ...............\n";
        //std::cout.flush();
        break;

      // Connection breaked.
      case boost::asio::error::connection_aborted:
      case boost::asio::error::connection_reset:
      case boost::asio::error::connection_refused:
        break;

      // Other error.
      case boost::asio::error::timed_out:
      	{
          boost::posix_time::time_duration time_long = boost::posix_time::microsec_clock::universal_time() - time_start;
          std::cout << "server error " << e << " message " << e.message() << "\n";
          std::cout << "time is " << time_long.total_milliseconds() << " ms.\n";
          std::cout.flush();
        }
        break;
      	
      case boost::asio::error::no_buffer_space:
      default:
        std::cout << "server error " << e << " message " << e.message() << "\n";
        std::cout.flush();
        break;
    }
  }

  void on_parent(server_handler_type& handler, const bas::event event)
  {
  }

  void on_child(server_handler_type& handler, const bas::event event)
  {
  }

private:
  boost::posix_time::ptime time_start;
};

} // namespace echo

#endif // BAS_ECHO_SERVER_WORK_HPP
