//
// win_main.cpp
// ~~~~~~~~~~~~

#if defined(_WIN32)

#ifndef _WINDOWS_
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  undef WIN32_LEAN_AND_MEAN
# else  // #ifndef WIN32_LEAN_AND_MEAN
#  include <windows.h>
# endif // #ifndef WIN32_LEAN_AND_MEAN
#endif  // #ifndef _WINDOWS_

#include <iostream>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "server_main.hpp"

std::string echo_service_name = "echo_server";

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

int main(int argc, char * argv[ ])
{
  if (argc == 2)
  {
    server_main server(argv[1]);

    // Set console control handler to allow server to be stopped.
    console_ctrl_function = boost::bind(&server_main::stop, &server);
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);

    // Run the server until stopped.
    server.run();

    return 0;
  }

  std::cerr << "Usage: echo_server [config_file]\n";
  return 1;
}

#endif // defined(_WIN32)
