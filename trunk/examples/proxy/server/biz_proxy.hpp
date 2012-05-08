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

class bgs_proxy
{
public:
  bgs_proxy() {}
  bgs_proxy(boost::asio::ip::tcp::endpoint& endpoint)
    : endpoint_(endpoint)
  {
  }

  boost::asio::ip::tcp::endpoint endpoint_;
};

/// The demo class for handle echo_server business process.
template<typename Global_Storage>
class biz_proxy
{
public:
  /// Constructor.
  biz_proxy(Global_Storage& bgs)
    : bgs_(bgs)
  {
  }

  /// Main function to process business logical operations.
  void process(status_t& status, io_buffer& input, io_buffer& output)
  {
    switch (status.state)
    {
      case BAS_STATE_ON_OPEN:
        status.state = BAS_STATE_DO_CLIENT_OPEN;
        status.endpoint = bgs_.endpoint_;
        break;

      case BAS_STATE_ON_CLIENT_OPEN:
        status.state = BAS_STATE_DO_READ;
        break;

      case BAS_STATE_ON_READ:
        status.state = BAS_STATE_DO_CLIENT_WRITE_READ;
        break;

     case BAS_STATE_ON_CLIENT_READ:
        if (status.ec)
        {
          status.state = BAS_STATE_DO_CLOSE;
        }
        else
          status.state = BAS_STATE_DO_WRITE;

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

  /// Global storage can be used in process for holding other application resources.
  Global_Storage& bgs_;
};

} // namespace proxy

#endif // BAS_BIZ_PROXY_HPP
