//
// main_client.cpp
// ~~~~~~~~~~~~~~~
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

#include "ssl_connections.hpp"

int main(int argc, char* argv[])
{
  try
  {
    // Check command line arguments.
    if (argc != 10)
    {
      std::cerr << "Usage: ssl_client <address> <port> <io_pool_size> <work_pool_init_size> <work_pool_high_watermark> <preallocated_handler_number> <data_buffer_size> <timeout_seconds> <connection_number>\n";
      std::cerr << "  For IPv4, try:\n";
      std::cerr << "    ssl_client 0.0.0.0 1000 4 4 16 500 64 0 100\n";
      std::cerr << "  For IPv6, try:\n";
      std::cerr << "    ssl_client 0::0 1000 4 4 16 500 64 0 100\n";
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
    std::size_t connection_number = boost::lexical_cast<std::size_t>(argv[9]);

    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;
    typedef bas::service_handler_pool<echo::ssl_client_work, echo::ssl_client_work_allocator, ssl_socket> ssl_client_handler_pool;

    echo::ssl_connections client(new ssl_client_handler_pool(new echo::ssl_client_work_allocator(),
                                     preallocated_handler_number,
                                     read_buffer_size,
                                     0,
                                     timeout_seconds),
        boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(argv[1]), port),
        io_pool_size,
        work_pool_init_size,
        work_pool_high_watermark,
        connection_number);

    // Run the client until stopped.
    client.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "exception: " << e.what() << "\n";
  }

  return 0;
}
