//
// ssl_client_work_allocator.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2009 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BAS_ECHO_CLIENT_WORK_ALLOCATOR_HPP
#define BAS_ECHO_CLIENT_WORK_ALLOCATOR_HPP

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/shared_ptr.hpp>

#include "ssl_client_work.hpp"

namespace echo {

class ssl_client_work_allocator
{
public:
  typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;

  ssl_client_work_allocator()
    : context_()
  {
  }

  ssl_socket* make_socket(boost::asio::io_service& io_service)
  {
    if (context_.get() == 0)
    {
       context_.reset(new boost::asio::ssl::context(io_service, boost::asio::ssl::context::sslv23));
       context_->set_verify_mode(boost::asio::ssl::context::verify_peer);
       context_->load_verify_file("ca.pem");
    }

    return new ssl_socket(io_service, *context_);
  }

  ssl_client_work* make_handler()
  {
    return new ssl_client_work();
  }

private:
  boost::shared_ptr<boost::asio::ssl::context> context_;
};

} // namespace echo

#endif // BAS_ECHO_CLIENT_WORK_ALLOCATOR_HPP
