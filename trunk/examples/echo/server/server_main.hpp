//
// server_main.cpp
// ~~~~~~~~~~~~~~~

#ifndef ECHO_SERVER_MAIN_HPP
#define ECHO_SERVER_MAIN_HPP

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <bas/server.hpp>

#include "config.hpp"
#include "server_base.hpp"
#include "app_param.hpp"
#include "server_work.hpp"
#include "server_work_allocator.hpp"

using namespace echo;

class server_main
  : public server_base,
    private boost::noncopyable
{
public:
  typedef bas::server<server_work, server_work_allocator> server;
  typedef bas::service_handler_pool<server_work, server_work_allocator> server_handler_pool;
  typedef boost::shared_ptr<server> server_ptr;

  server_main(const std::string& config_file)
    : config_file_(config_file),
      server_()
  {
  }

  ~server_main()
  {
    server_.reset();
  }

  void run()
  {
    if (init() == ECHO_ERROR_NONE)
    {
      // Run the server until stopped.
      server_->run();
    }
  }
  
  int start(DWORD argc, LPTSTR *argv)
  {
    int ret = init();

    if (ret != ECHO_ERROR_NONE)
      return ret;

    // Run the server with nonblock mode.
    server_->start();

    return ret;
  }
    
  void start(void)
  {
    start(0, NULL);
  }

  void stop()
  {
    if (server_.get() != 0)
      server_->stop();
  }

private:

  int init(void)
  {
    if (server_.get() != 0)
      return ECHO_ERROR_NONE;

    int ret = get_param(config_file_, param_);
    if (ret != ECHO_ERROR_NONE)
      return ret;

    server_.reset(new server(new server_handler_pool(new server_work_allocator(),
                                                     param_.handler_pool_init,
                                                     param_.read_buffer_size,
                                                     param_.write_buffer_size,
                                                     param_.session_timeout,
                                                     param_.io_timeout,
                                                     param_.handler_pool_low,
                                                     param_.handler_pool_high,
                                                     param_.handler_pool_inc,
                                                     param_.handler_pool_max),
                             param_.ip,
                             param_.port,
                             param_.io_thread_size,
                             param_.work_thread_init,
                             param_.work_thread_high,
                             param_.work_thread_load,
                             param_.accept_queue_size));

    if (server_.get() == 0)
      return ECHO_ERROR_ALLOC_FAILED;

    return ECHO_ERROR_NONE;
  }

private:

  /// The config file of server.
  std::string config_file_;

  /// The application parameters of server.
  app_param param_;

  /// The pointer of server.
  server_ptr server_; 
};

#endif // ECHO_SERVER_MAIN_HPP

