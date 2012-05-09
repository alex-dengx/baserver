//
// biz_proxy.hpp
// ~~~~~~~~~~~~~
//
// Copyright (c) 2009 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BAS_BIZ_PROXY_HPP
#define BAS_BIZ_PROXY_HPP

#include <bastool/server_work.hpp>

namespace proxy {

using namespace bastool;

/// Business global storage for proxy.
class bgs_proxy
{
public:
  /// Constructor.
  bgs_proxy(const std::string& address,
      unsigned short port)
    : endpoint_(boost::asio::ip::address::from_string(address), port)
  {
  }

  /// Not used here.
  void init()  {}
  void close() {}

  /// Target server address.
  boost::asio::ip::tcp::endpoint endpoint_;
};

/// Class for handle proxy_server business process.
template<typename Biz_Global_Storage>
class biz_proxy
{
public:
  typedef boost::shared_ptr<Biz_Global_Storage> bgs_ptr;

  /// Constructor.
  biz_proxy(bgs_ptr bgs)
    : bgs_(bgs)
  {
    BOOST_ASSERT(bgs_.get () != 0);
  }

  /// Main function to process business logical operations.
  void process(status_t& status, io_buffer& input, io_buffer& output)
  {
    switch (status.state)
    {
      case BAS_STATE_ON_OPEN:
        status.state = BAS_STATE_DO_CLIENT_OPEN;
        status.endpoint = bgs_->endpoint_;
        break;

      case BAS_STATE_ON_CLIENT_OPEN:
        status.state = BAS_STATE_DO_READ;
        break;

      case BAS_STATE_ON_READ:
        status.state = BAS_STATE_DO_CLIENT_WRITE_READ;
        break;

     case BAS_STATE_ON_CLIENT_READ:
        if (status.ec || output.space() < input.size())
        {
          status.state = BAS_STATE_DO_CLOSE;
        }
        else
        {
          output.clear();
          memcpy(output.data(), input.data(), input.size());
          status.state = BAS_STATE_DO_WRITE;
        }

        break;

     case BAS_STATE_ON_CLIENT_WRITE:
        status.state = BAS_STATE_DO_CLOSE;
        break;

      case BAS_STATE_ON_WRITE:
        status.state = BAS_STATE_DO_READ;
        break;

      case BAS_STATE_ON_CLOSE:
      case BAS_STATE_ON_CLIENT_CLOSE:
        switch (status.ec.value())
        {
          // Operation successfully completed.
          case 0:
          case boost::asio::error::eof:
            break;

          // Connection breaked.
          case boost::asio::error::connection_aborted:
          case boost::asio::error::connection_reset:
          case boost::asio::error::connection_refused:
            std::cout << "C";
            break;

          // Connection timeout.
          case boost::asio::error::timed_out:
            std::cout << "T";
            break;
        
          case boost::asio::error::no_buffer_space:
          default:
            std::cout << "O";
            break;
        }

        status.state = BAS_STATE_DO_CLOSE;
        break;

      default:
        status.state = BAS_STATE_DO_CLOSE;
        break;
    }
  }

  /// Business Global storage for holding application resources.
  bgs_ptr bgs_;
};

} // namespace proxy

#endif // BAS_BIZ_PROXY_HPP
