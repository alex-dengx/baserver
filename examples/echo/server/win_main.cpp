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

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <bastool/win_service.hpp>
#include <iostream>

#include "server_main.hpp"

std::string service_name = "echo_server";
std::string display_name = "echo_server";
std::string description  = "echo server";

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
  if (argc >= 2)
  {
    if (memcmp(argv[1], "/service", 8) == 0 && argc == 3)
    {
      server_main server(argv[2]);
      bastool::win_service service(&server, service_name);
      service.run(&service);
      return 0;
    }
    else if (memcmp(argv[1], "/install", 8) == 0 && argc == 3)
    {
      char bin_path[MAX_PATH];
      memcpy(bin_path, "/service ", 9);
      memcpy(bin_path + 9, argv[2], strlen(argv[2]) + 1);
      DWORD ret = bastool::win_service::install(service_name, display_name, description, bin_path);
      if (ret == 0)
      {
        std::cout << "Service " << service_name << " install success.\n";
        return 0;
      }
      else
      {
        std::cerr << "Service " << service_name << " install failed. errno = " << ret << "\n";
        return ret;
      }
    }
    else if (memcmp(argv[1], "/delete", 7) == 0)
    {
      DWORD ret = bastool::win_service::remove(service_name);
      if (ret == 0)
      {
        std::cout << "Service " << service_name << " delete success.\n";
        return 0;
      }
      else
      {
        std::cerr << "Service " << service_name << " delete failed. errno = " << ret << "\n";
        return ret;
      }
    }
    else if (argc == 2 && memcmp(argv[1], "/install", 8) != 0)
    {
      server_main server(argv[1]);

      // Set console control handler to allow server to be stopped.
      console_ctrl_function = boost::bind(&server_main::stop, &server);
      SetConsoleCtrlHandler(console_ctrl_handler, TRUE);

      // Run the server until stopped.
      server.run();

      return 0;
    }
  }

  std::cerr << "Usage: " << service_name << " <arguments>\n";
  std::cerr << "  " << service_name << " [/install] [config_file] : install " << service_name << " service.\n";
  std::cerr << "  " << service_name << " [/delete]                : delete " << service_name << " service.\n";
  std::cerr << "  " << service_name << " [config_file]            : run as application.\n";
  return 1;
}

#endif // defined(_WIN32)
