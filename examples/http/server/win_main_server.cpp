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
    if (argc != 9)
    {
      std::cerr << "Usage: http_server <address> <port> <io_pool_size> <thread_pool_size> <preallocated_handler_number> <timeout_seconds> <closed_wait> <doc_root>\n";
      std::cerr << "  For IPv4, try:\n";
      std::cerr << "    http_server 0.0.0.0 80 4 4 500 0 5 .\n";
      std::cerr << "  For IPv6, try:\n";
      std::cerr << "    http_server 0::0 80 4 4 500 0 5 .\n";
      return 1;
    }

    // Initialise server.
    std::size_t io_pool_size = boost::lexical_cast<std::size_t>(argv[3]);
    std::size_t thread_pool_size = boost::lexical_cast<std::size_t>(argv[4]);
    std::size_t preallocated_handler_number = boost::lexical_cast<std::size_t>(argv[5]);
    std::size_t timeout_seconds = boost::lexical_cast<std::size_t>(argv[6]);
    std::size_t closed_wait = boost::lexical_cast<std::size_t>(argv[7]);

    typedef bas::server<http::server::server_work, http::server::server_work_allocator> server;
    typedef bas::service_handler_pool<http::server::server_work, http::server::server_work_allocator> server_handler_pool;

    server s(argv[1],
        argv[2],
        io_pool_size,
        thread_pool_size,
        new server_handler_pool(new http::server::server_work_allocator(argv[8]),
            preallocated_handler_number,
            8192,
            0,
            timeout_seconds,
            closed_wait));

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
