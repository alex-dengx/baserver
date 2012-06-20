//
// main_client.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

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
    if (argc != 11)
    {
      std::cerr << "Usage: echo_client <address> <port> <io_pool_size> <handler_pool_init> <buffer_size> <timeout_milliseconds> <pause_seconds> <connection_number> <wait_seconds> <test_times>\n";
      std::cerr << "  For IPv4, try:\n";
      std::cerr << "    echo_client 127.0.0.1 1000 4 100 64 2000 3 1000 10 10\n";
      std::cerr << "  For IPv6, try:\n";
      std::cerr << "    echo_client 0::0 1000 4 100 64 2000 3 1000 10 10\n";
      return 1;
    }

    using namespace boost::asio::ip;

    // Initialise client.
    unsigned short port = boost::lexical_cast<unsigned short>(argv[2]);
    std::size_t io_pool_size = boost::lexical_cast<std::size_t>(argv[3]);
    std::size_t preallocated_handler_number = boost::lexical_cast<std::size_t>(argv[4]);
    std::size_t buffer_size = boost::lexical_cast<std::size_t>(argv[5]);
    long timeout_milliseconds = boost::lexical_cast<long>(argv[6]);
    unsigned int pause_seconds = boost::lexical_cast<unsigned int >(argv[7]);
    std::size_t connection_number = boost::lexical_cast<std::size_t>(argv[8]);
    unsigned int wait_seconds = boost::lexical_cast<unsigned int >(argv[9]);
    unsigned int test_times = boost::lexical_cast<unsigned int >(argv[10]);

    bas::endpoint_group endpoints;
    endpoints.set(tcp::endpoint(address::from_string(argv[1]), port));

    echo::error_count counter;
    
    echo::connections client(io_pool_size,
                             endpoints,
                             buffer_size,
                             timeout_milliseconds,
                             preallocated_handler_number,
                             counter,
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
