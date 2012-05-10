//
// server_main.cpp
// ~~~~~~~~~~~~~~~

#ifndef PROXY_SERVER_MAIN_HPP
#define PROXY_SERVER_MAIN_HPP

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <bas/server.hpp>

#include <bastool/server_work.hpp>
#include <bastool/server_work_allocator.hpp>
#include <bastool/server_base.hpp>

#include "config.hpp"
#include "app_param.hpp"
#include "biz_proxy.hpp"

using namespace bas;
using namespace bastool;
using namespace proxy;

class server_main
  : public server_base,
    private boost::noncopyable
{
public:
  typedef biz_proxy<bgs_proxy> biz_handler_t;
  typedef server_work<biz_handler_t> server_work_t;
  typedef server_work_allocator<biz_handler_t, bgs_proxy> server_work_allocator_t;
  typedef client_work<biz_handler_t> client_work_t;
  typedef client_work_allocator<biz_handler_t> client_work_allocator_t;

  typedef server<server_work_t, server_work_allocator_t> server_t;
  typedef client<client_work_t, client_work_allocator_t> client_t;
  typedef service_handler_pool<server_work_t, server_work_allocator_t> server_handler_pool_t;
  typedef service_handler_pool<client_work_t, client_work_allocator_t> client_handler_pool_t;

  typedef boost::shared_ptr<server_t> server_ptr;

  /// Constructor.
  server_main(const std::string& config_file)
    : config_file_(config_file),
      server_()
  {
  }

  ~server_main()
  {
    server_.reset();
  }

  /// Run the server with block mode.
  void run()
  {
    if (init() == PROXY_ERR_NONE)
    {
      // Run the server until stopped.
      server_->run();
    }
  }

  /// Start the server with nonblock mode.
  int start(DWORD argc, LPTSTR *argv)
  {
    int ret = init();

    if (ret != PROXY_ERR_NONE)
      return ret;

    // Run the server with nonblock mode.
    server_->start();

    return 0;
  }
    
  /// Start the server with nonblock mode.
  void start(void)
  {
    start(0, NULL);
  }

  /// Stop the server.
  void stop()
  {
    if (server_.get() != 0)
      server_->stop();
  }

private:
  /// Initialize the server objects.
  int init(void)
  {
    if (server_.get() != 0)
      return PROXY_ERR_NONE;

    int ret = get_param(config_file_, param_);
    if (ret != PROXY_ERR_NONE)
      return ret;

    bgs_proxy* bgs = new bgs_proxy(param_.proxy_ip, param_.proxy_port);

    client_t* client = new client_t(new client_handler_pool_t(new client_work_allocator_t(),
                                                              param_.handler_pool_init,
                                                              param_.read_buffer_size,
                                                              param_.write_buffer_size,
                                                              param_.session_timeout,
                                                              param_.io_timeout,
                                                              param_.handler_pool_low,
                                                              param_.handler_pool_high,
                                                              param_.handler_pool_inc,
                                                              param_.handler_pool_max));

    server_.reset(new server_t(new server_handler_pool_t(new server_work_allocator_t(bgs, client),
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
      return PROXY_ERR_ALLOC_FAILED;

    return PROXY_ERR_NONE;
  }

private:
  /// The config file of server.
  std::string config_file_;

  /// The application parameters of server.
  app_param param_;

  /// The pointer of server.
  server_ptr server_; 
};

#endif // PROXY_SERVER_MAIN_HPP

