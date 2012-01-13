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

#include <bas/server.hpp>

#include "server_work.hpp"
#include "server_work_allocator.hpp"

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
    if (argc != 11)
    {
      std::cerr << "Usage: http_server <ip> <port> <io_pool> <work_init> <work_high> <thread_load> <accept_queue> <pre_handler> <session_timeout> <doc_root>\n";
      std::cerr << "  For IPv4, try:\n";
      std::cerr << "    http_server 0.0.0.0 80 4 4 16 100 250 500 0 .\n";
      std::cerr << "  For IPv6, try:\n";
      std::cerr << "    http_server 0::0 80 4 4 16 100 250 500 0 .\n";
      return 1;
    }

    // Initialise server.
    unsigned short port = boost::lexical_cast<unsigned short>(argv[2]);
    std::size_t io_pool_size = boost::lexical_cast<std::size_t>(argv[3]);
    std::size_t work_pool_init_size = boost::lexical_cast<std::size_t>(argv[4]);
    std::size_t work_pool_high_watermark = boost::lexical_cast<std::size_t>(argv[5]);
    std::size_t work_pool_thread_load = boost::lexical_cast<std::size_t>(argv[6]);
    std::size_t accept_queue_length = boost::lexical_cast<std::size_t>(argv[7]);
    std::size_t preallocated_handler_number = boost::lexical_cast<std::size_t>(argv[8]);
    std::size_t session_timeout = boost::lexical_cast<std::size_t>(argv[9]);

    typedef bas::server<http::server::server_work, http::server::server_work_allocator> server;
    typedef bas::service_handler_pool<http::server::server_work, http::server::server_work_allocator> server_handler_pool;

    server s(new server_handler_pool(new http::server::server_work_allocator(argv[10]),
                preallocated_handler_number,
                8192,
                0,
                session_timeout),
        argv[1],
        port,
        io_pool_size,
        work_pool_init_size,
        work_pool_high_watermark,
        work_pool_thread_load,
        accept_queue_length);

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
