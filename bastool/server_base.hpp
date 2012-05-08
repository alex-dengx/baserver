//
// server_base.hpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2009 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BASTOOL_SERVER_BASE_HPP
#define BASTOOL_SERVER_BASE_HPP

namespace bastool {

/// The base class for general server.
class server_base
{
public:

  server_base()
  {
  }

  ~server_base()
  {
  }

  /// Executes when startup.
  virtual int start(DWORD argc, LPTSTR *argv) = 0;

  /// Executes when continue.
  virtual void start(void) = 0;

  /// Executes when custom command.
  virtual void do_command(DWORD controls) {}

  /// Executes when stop.
  virtual void stop(void) = 0;
};

} // namespace bastool

#endif  // BASTOOL_SERVER_BASE_HPP
