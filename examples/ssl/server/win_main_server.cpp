//
// win_main_server.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#define BOOST_LIB_DIAGNOSTIC

#include <iostream>
#include <string>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <bas/server.hpp>

#include "ssl_server_work.hpp"
#include "ssl_server_work_allocator.hpp"

#if defined(_WIN32)

boost::function0<void> console_ctrl_function;

BOOL WINAPI console_ctrl_handler(DWORD ctrl_type)
{
  switch (ctrl_type)
  {
  case CTRL_C_EVENT:
  case CTRL_BREAK_EVENT:
  case CTRL_CLOSE_EVENT:
  case CTRL_SHUTDOWN_EVENT:
    console_ctrl_function();
    return TRUE;
  default:
    return FALSE;
  }
}

int main(int argc, char* argv[])
{
  try
  {
    // Check command line arguments.
    if (argc != 9)
    {
      std::cerr << "Usage: ssl_server <address> <port> <io_pool_size> <work_pool_init_size> <work_pool_high_watermark> <preallocated_handler_number> <data_buffer_size> <timeout_seconds>\n";
      std::cerr << "  For IPv4, try:\n";
      std::cerr << "    ssl_server 0.0.0.0 1000 4 4 16 500 256 0\n";
      std::cerr << "  For IPv6, try:\n";
      std::cerr << "    ssl_server 0::0 1000 4 4 16 500 256 0\n";
      return 1;
    }

    // Initialise server.
    unsigned short port = boost::lexical_cast<unsigned short>(argv[2]);
    std::size_t io_pool_size = boost::lexical_cast<std::size_t>(argv[3]);
    std::size_t work_pool_init_size = boost::lexical_cast<std::size_t>(argv[4]);
    std::size_t work_pool_high_watermark = boost::lexical_cast<std::size_t>(argv[5]);
    std::size_t preallocated_handler_number = boost::lexical_cast<std::size_t>(argv[6]);
    std::size_t read_buffer_size = boost::lexical_cast<std::size_t>(argv[7]);
    std::size_t timeout_seconds = boost::lexical_cast<std::size_t>(argv[8]);

    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;
    typedef bas::server<echo::ssl_server_work, echo::ssl_server_work_allocator, ssl_socket> server;
    typedef bas::service_handler_pool<echo::ssl_server_work, echo::ssl_server_work_allocator, ssl_socket> server_handler_pool;

    server s(new server_handler_pool(new echo::ssl_server_work_allocator(),
                 preallocated_handler_number,
                 read_buffer_size,
                 0,
                 timeout_seconds),
        argv[1],
        port,
        io_pool_size,
        work_pool_init_size,
        work_pool_high_watermark);

    // Set console control handler to allow server to be stopped.
    console_ctrl_function = boost::bind(&server::stop, &s);
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);

    // Run the server until stopped.
    s.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "exception: " << e.what() << "\n";
  }

  return 0;
}

#endif // defined(_WIN32)
