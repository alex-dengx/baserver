//
// main_client.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

//#define BOOST_LIB_DIAGNOSTIC

#include <iostream>
#include <string>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>

#include "connections.hpp"

int main(int argc, char* argv[])
{
  try
  {
    // Check command line arguments.
    if (argc != 14)
    {
      std::cerr << "Usage: echo_client <address> <port> <io_pool> <work_pool_init> <work_pool_high> <handler_pool_init> <data_buffer_size> <session_timeout> <io_timeout> <pause_seconds> <connection_number> <wait_seconds> <test_times>\n";
      std::cerr << "  For IPv4, try:\n";
      std::cerr << "    echo_client 127.0.0.1 1000 4 4 16 100 64 30 0 3 1000 10 10\n";
      std::cerr << "  For IPv6, try:\n";
      std::cerr << "    echo_client 0::0 1000 4 4 16 100 64 30 0 3 1000 10 10\n";
      return 1;
    }

    // Initialise server.
    unsigned short port = boost::lexical_cast<unsigned short>(argv[2]);
    std::size_t io_pool_size = boost::lexical_cast<std::size_t>(argv[3]);
    std::size_t work_pool_init_size = boost::lexical_cast<std::size_t>(argv[4]);
    std::size_t work_pool_high_watermark = boost::lexical_cast<std::size_t>(argv[5]);
    std::size_t preallocated_handler_number = boost::lexical_cast<std::size_t>(argv[6]);
    std::size_t read_buffer_size = boost::lexical_cast<std::size_t>(argv[7]);
    unsigned int session_timeout = boost::lexical_cast<unsigned int>(argv[8]);
    unsigned int io_timeout = boost::lexical_cast<unsigned int>(argv[9]);
    unsigned int pause_seconds = boost::lexical_cast<unsigned int >(argv[10]);
    std::size_t connection_number = boost::lexical_cast<std::size_t>(argv[11]);
    unsigned int wait_seconds = boost::lexical_cast<unsigned int >(argv[12]);
    unsigned int test_times = boost::lexical_cast<unsigned int >(argv[13]);

    typedef bas::service_handler_pool<echo::client_work, echo::client_work_allocator> client_handler_pool;
    echo::error_count counter;

    echo::connections client(new client_handler_pool(new echo::client_work_allocator(&counter, pause_seconds),
                                 preallocated_handler_number,
                                 read_buffer_size,
                                 0,
                                 session_timeout,
                                 io_timeout),
        &counter,
        argv[1],
        port,
        io_pool_size,
        work_pool_init_size,
        work_pool_high_watermark,
        pause_seconds,
        connection_number,
        wait_seconds,
        test_times);

    // Run the client until stopped.
    client.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "exception: " << e.what() << "\n";
  }

  return 0;
}
