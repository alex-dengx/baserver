//
// win_service.hpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2009 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BASTOOL_WIN_SERVICE_HPP
#define BASTOOL_WIN_SERVICE_HPP

#ifndef _WINDOWS_
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  undef WIN32_LEAN_AND_MEAN
# else  // #ifndef WIN32_LEAN_AND_MEAN
#  include <windows.h>
# endif // #ifndef WIN32_LEAN_AND_MEAN
#endif  // #ifndef _WINDOWS_

#include <boost/assert.hpp>
#include <bastool/hash_map.hpp>
#include <bastool/server_base.hpp>
#include <string>
#include <memory>

namespace bastool {

/// The general windows service class.
class win_service
{
public:
  typedef bastool::hash_map<std::string, win_service> service_table_t;

  win_service(server_base* server, const std::string& service_name)
    : server_(server),
      service_name_(service_name),
      status_handle_(0)
  {
    BOOST_ASSERT(server != 0);

    init_service_status();
  }

  ~win_service()
  {
  }

  /// Starts up the service and does not return until service have entered the SERVICE_STOPPED state.
  bool run(win_service* service)
  {
    if (service == 0)
      return false;

    // Save the service into hash map.
    services_.insert_update(service->get_service_name(), *service);

    // Use ANSI version.
    SERVICE_TABLE_ENTRYA service_table[] = {
      {(char *)service->get_service_name().c_str(), (LPSERVICE_MAIN_FUNCTIONA)service_main},
      {0, 0}
    };

    // Use ANSI version.
    DWORD ret = ::StartServiceCtrlDispatcherA(service_table);

    // Erase the service from hash map.
    services_.erase(service->get_service_name());

    return (ret != 0);
  }
  
  static DWORD install(const std::string& service_name,
                       const std::string& display_name,
                       const std::string& description,
                       const std::string& arguments)
  {
    SC_HANDLE manager_handle;
    SC_HANDLE service_handle;
    SERVICE_DESCRIPTIONA sd_a;
    char bin_path[MAX_PATH];
    DWORD ret = 0;

    // Get the path of current process, use ANSI version.
    if (!::GetModuleFileNameA(NULL, bin_path, MAX_PATH))
      return ::GetLastError();

    if (arguments.length() != 0)
    {
      std::size_t mlen = strlen(bin_path);
      if (mlen + 1 + arguments.length() + 1 > MAX_PATH)
        return ERROR_INVALID_PARAMETER;

      bin_path[mlen] = 0x20;
      memcpy(bin_path + mlen + 1, arguments.c_str(), arguments.length() + 1);
    }

    // Get a handle to the SCM database, use ANSI version.
    manager_handle = ::OpenSCManagerA(NULL,                    // local computer
                                      NULL,                    // ServicesActive database
                                      SC_MANAGER_ALL_ACCESS);  // full access rights

    if (manager_handle == NULL)
      return ::GetLastError();

    // Create the service, use ANSI version.
    service_handle = ::CreateServiceA(manager_handle,            // SCM database
                                      service_name.c_str(),      // name of service
                                      display_name.c_str(),      // service name to display
                                      SERVICE_ALL_ACCESS,        // desired access
                                      SERVICE_WIN32_OWN_PROCESS, // service type
                                      SERVICE_DEMAND_START,      // start type
                                      SERVICE_ERROR_NORMAL,      // error control type
                                      bin_path,                  // path to service's binary
                                      NULL,                      // no load ordering group
                                      NULL,                      // no tag identifier
                                      NULL,                      // no dependencies
                                      NULL,                      // LocalSystem account
                                      NULL);                     // no password
 
    if (service_handle == NULL)
    {
      ret = ::GetLastError();
      ::CloseServiceHandle(manager_handle);

      return ret;
    }

    // Change description, use ANSI version.
    sd_a.lpDescription = (char *)description.c_str();
    if (!::ChangeServiceConfig2A(service_handle,              // handle to service
                                 SERVICE_CONFIG_DESCRIPTION,  // change: description
                                 &sd_a))                      // new description
      ret = ::GetLastError();

    ::CloseServiceHandle(service_handle);
    ::CloseServiceHandle(manager_handle);
    
    return ret;
  }

  static DWORD remove(const std::string& service_name)
  {
    SC_HANDLE manager_handle;
    SC_HANDLE service_handle;
    DWORD ret = 0;

    // Get a handle to the SCM database, use ANSI version.
    manager_handle = ::OpenSCManager(NULL,                    // local computer
                                     NULL,                    // ServicesActive database
                                     SC_MANAGER_ALL_ACCESS);  // full access rights
 
    if (manager_handle == NULL)
      return ::GetLastError();
  
    // Get a handle to the service, use ANSI version.
    service_handle = ::OpenServiceA(manager_handle,         // SCM database
                                    service_name.c_str(),  // name of service
                                    DELETE);                // need delete access
 
    if (service_handle == NULL)
    {
      ret = ::GetLastError();
      ::CloseServiceHandle(manager_handle);

      return ret;
    }

    // Delete the service.
    if (!::DeleteService(service_handle))
       ret = ::GetLastError();
 
    ::CloseServiceHandle(service_handle); 
    ::CloseServiceHandle(manager_handle);

    return ret;
  }

  /// Set the type of service property value.
  ///   Can be one of the following values.
  ///     SERVICE_FILE_SYSTEM_DRIVER   The service is a file system driver.
  ///     SERVICE_KERNEL_DRIVER        The service is a device driver.
  ///     SERVICE_WIN32_OWN_PROCESS    The service runs in its own process.
  ///     SERVICE_WIN32_SHARE_PROCESS  The service shares a process with other services.
  ///   For mostly, value may be SERVICE_WIN32 = SERVICE_WIN32_OWN_PROCESS | SERVICE_WIN32_SHARE_PROCESS.
  ///   Default value is SERVICE_WIN32_OWN_PROCESS.
  void set_service_type(DWORD type)
  {
    status_.dwServiceType = type;
  }

  /// Get the type of service property value.
  DWORD get_service_type()
  {
    return status_.dwServiceType;
  }

  /// Set the estimated time required for a pending start, stop, pause, or continue operation, in milliseconds.
  void set_wait_hint(DWORD start_wait = 0,
                     DWORD continue_wait = 0,
                     DWORD pause_wait = 0,
                     DWORD stop_wait = 0)
  {
    start_wait_ = start_wait;
    continue_wait_ = continue_wait;
    pause_wait_ = pause_wait;
    stop_wait_ = stop_wait;
  }

  /// Get the control codes the service accepts and processes in its handler function.
  DWORD get_controls(void)
  {
    return status_.dwControlsAccepted;
  }

  /// Set the control codes the service accepts and processes in its handler function.
  void set_controls(DWORD controls)
  {
    if (status_handle_ != 0)
      status_.dwControlsAccepted |= controls;
  }

  /// Clear the control codes the service accepts and processes in its handler function.
  void clear_controls(DWORD controls)
  {
    if (status_handle_ != 0)
      status_.dwControlsAccepted ^= controls;
  }

  /// Check the control codes the service accepts and processes in its handler function.
  bool check_controls(DWORD controls)
  {
    return ((status_.dwControlsAccepted & controls) != 0);
  }

  /// Updates the service control manager's whole status information for the calling service.
  bool update_service_status(void)
  {
    return (::SetServiceStatus(status_handle_, &status_) != 0);
  }

  /// Updates the service control manager's dwCurrentState and dwWin32ExitCode information for the calling service.
  bool update_service_state(DWORD state, DWORD exit_code = NO_ERROR)
  {
    if (status_.dwCurrentState != state)
    {
      switch (state)
      {
        case SERVICE_START_PENDING:
          status_.dwWaitHint = start_wait_;
          break;

        case SERVICE_CONTINUE_PENDING:
          status_.dwWaitHint = continue_wait_;
          break;

        case SERVICE_STOP_PENDING:
          status_.dwWaitHint = stop_wait_;
          break;

        case SERVICE_PAUSE_PENDING:
          status_.dwWaitHint = pause_wait_;
          break;

        default:
          status_.dwWaitHint = 0;
          break;
      }

      status_.dwCurrentState = state;
      status_.dwCheckPoint = 0;
    }
    else
    {
      switch (state)
      {
        case SERVICE_START_PENDING:
        case SERVICE_CONTINUE_PENDING:
        case SERVICE_STOP_PENDING:
        case SERVICE_PAUSE_PENDING:
          status_.dwCheckPoint++;
          break;

        default:
          status_.dwWaitHint = 0;
          status_.dwCheckPoint = 0;
          break;
      }
    }

    status_.dwWin32ExitCode = exit_code;

    return update_service_status();
  }

  /// Gets the short name used to identify the service to the system.
  const std::string& get_service_name(void) const
  {
    return service_name_;
  }

  /// Sets the short name used to identify the service to the system.
  void set_service_name(const std::string& name)
  {
    service_name_ = name;
  }
  
  /// Gets the server object.
  server_base& get_server(void) const
  {
    BOOST_ASSERT(server_ != 0);

    return *server_;
  }

protected:
  /// The callback function used as the control handler for the service.
  static DWORD WINAPI win_service_handler(DWORD controls,
                                          DWORD event_type,
                                          LPVOID event_data,
                                          LPVOID context)
  {
    win_service* service = (win_service*)context;

    if (service == 0)
      return NO_ERROR;

    switch(controls)
    {
      case SERVICE_CONTROL_PAUSE:
        service->update_service_state(SERVICE_PAUSE_PENDING);
        service->get_server().stop();
        service->update_service_state(SERVICE_PAUSED);
        break;

      case SERVICE_CONTROL_CONTINUE:
        service->update_service_state(SERVICE_CONTINUE_PENDING);
        service->get_server().start();
        service->update_service_state(SERVICE_RUNNING);
        break;

      case SERVICE_CONTROL_STOP:
      case SERVICE_CONTROL_SHUTDOWN:
        service->update_service_state(SERVICE_STOP_PENDING);
        service->get_server().stop();
        service->update_service_state(SERVICE_STOPPED);
        break;

      case SERVICE_CONTROL_INTERROGATE:
        service->update_service_status();
        break;
      
      default:
        service->get_server().do_command(controls);
        break;
    }
  
    return NO_ERROR;
  }

  /// The entry point for the service.
  static VOID WINAPI service_main(DWORD argc, LPTSTR *argv)
  {
    win_service* service = services_.find((char*)argv[0]);

    if (service == 0)
      return;

    if (!service->register_service())
      return;

    service->update_service_state(SERVICE_START_PENDING);

    int ret = service->get_server().start(argc, argv);
    if (ret == 0)
    {
      service->set_controls(SERVICE_ACCEPT_STOP);
      service->update_service_state(SERVICE_RUNNING);
    }
    else
      service->update_service_state(SERVICE_STOPPED, ret);
  }

private:
  /// Set the initialization value of service status.
  void init_service_status(void)
  {
    memset(&status_, 0x00, sizeof(SERVICE_STATUS));
    status_.dwServiceType = SERVICE_WIN32_OWN_PROCESS;

    start_wait_    = 0;
    continue_wait_ = 0;
    pause_wait_    = 0;
    stop_wait_     = 0;
  }

  /// Registers the a function to handle extended service control requests.
  bool register_service(void)
  {
    // Use ANSI version.
    status_handle_ = ::RegisterServiceCtrlHandlerExA((LPCSTR)service_name_.c_str(),
                                                     (LPHANDLER_FUNCTION_EX)win_service_handler,
                                                     (LPVOID)this);

    return (status_handle_ != 0);
  }

private:
  /// The server object for providing service.
  server_base* server_;

  /// The static hash map for holding services.
  static service_table_t services_;

  /// The short name for the service.
  std::string service_name_;

  /// The handle to the status information structure for the service.
  SERVICE_STATUS_HANDLE status_handle_;

  /// The structure contains the status information for the service.
  SERVICE_STATUS status_;
  
  /// The estimated time required for a pending start, stop, pause, or continue operation, in milliseconds.
  DWORD start_wait_;
  DWORD continue_wait_;
  DWORD pause_wait_;
  DWORD stop_wait_;
};

/// Initialize the static hash map value. 
win_service::service_table_t win_service::services_(3, 3);

} // namespace bastool

#endif  // BASTOOL_WIN_SERVICE_HPP
