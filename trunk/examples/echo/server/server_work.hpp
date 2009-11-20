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
    std::cout << "connect ok ...............\n";
    std::cout.flush();

    //std::cout << "client ip is " << handler.socket().remote_endpoint().address().to_string() << "\n";
    //std::cout.flush();

    handler.async_read_some();
  }

  void on_read(server_handler_type& handler, std::size_t bytes_transferred)
  {
    std::cout << "receive " << bytes_transferred << " bytes ok ...............\n";
    std::cout.flush();

    handler.async_write(boost::asio::buffer(handler.read_buffer().data(), bytes_transferred));
  }

  void on_write(server_handler_type& handler, std::size_t bytes_transferred)
  {
    std::cout << "send " << bytes_transferred << " bytes ok ...............\n";
    std::cout.flush();

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
        std::cout << "close ok ...............\n";
        std::cout.flush();
        break;

      // Connection breaked.
      case boost::asio::error::connection_aborted:
      case boost::asio::error::connection_reset:
      case boost::asio::error::connection_refused:
        break;

      // Other error.
      case boost::asio::error::timed_out:
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
};

} // namespace echo

#endif // BAS_ECHO_SERVER_WORK_HPP
