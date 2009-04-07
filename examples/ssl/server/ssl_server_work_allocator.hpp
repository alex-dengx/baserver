//
// ssl_server_work_allocator.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2009 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BAS_SSL_SERVER_WORK_ALLOCATOR_HPP
#define BAS_SSL_SERVER_WORK_ALLOCATOR_HPP

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/shared_ptr.hpp>

#include "ssl_server_work.hpp"

namespace echo {

class ssl_server_work_allocator
{
public:
  typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;

  ssl_server_work_allocator()
    : context_()
  {
  }

  std::string get_password() const
  {
    return "test";
  }

  ssl_socket* make_socket(boost::asio::io_service& io_service)
  {
    if (context_.get() == 0)
    {
       context_.reset(new boost::asio::ssl::context(io_service, boost::asio::ssl::context::sslv23));
       context_->set_options(
           boost::asio::ssl::context::default_workarounds
           | boost::asio::ssl::context::no_sslv2
           | boost::asio::ssl::context::single_dh_use);
       context_->set_password_callback(boost::bind(&ssl_server_work_allocator::get_password, this));
       context_->use_certificate_chain_file("server.pem");
       context_->use_private_key_file("server.pem", boost::asio::ssl::context::pem);
       context_->use_tmp_dh_file("dh512.pem");
    }

    return new ssl_socket(io_service, *context_);
  }


  ssl_server_work* make_handler()
  {
    return new ssl_server_work();
  }

private:
  boost::shared_ptr<boost::asio::ssl::context> context_;
};

} // namespace echo

#endif // BAS_SSL_SERVER_WORK_ALLOCATOR_HPP
