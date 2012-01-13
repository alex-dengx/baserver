//
// posix_main_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#define BOOST_LIB_DIAGNOSTIC

#include <iostream>
#include <string>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

#include <bas/server.hpp>

#include "server_work.hpp"
#include "server_work_allocator.hpp"

#if !defined(_WIN32)

#include <pthread.h>
#include <signal.h>

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

    // Block all signals for background thread.
    sigset_t new_mask;
    sigfillset(&new_mask);
    sigset_t old_mask;
    pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);

    // Run server in background thread.
    boost::thread t(boost::bind(&server::run, &s));

    // Restore previous signals.
    pthread_sigmask(SIG_SETMASK, &old_mask, 0);

    // Wait for signal indicating time to shut down.
    sigset_t wait_mask;
    sigemptyset(&wait_mask);
    sigaddset(&wait_mask, SIGINT);
    sigaddset(&wait_mask, SIGQUIT);
    sigaddset(&wait_mask, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &wait_mask, 0);
    int sig = 0;
    sigwait(&wait_mask, &sig);

    // Stop the server.
    s.stop();
    t.join();
  }
  catch (std::exception& e)
  {
    std::cerr << "exception: " << e.what() << "\n";
  }

  return 0;
}

#endif // !defined(_WIN32)
