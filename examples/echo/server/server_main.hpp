//
// server_main.cpp
// ~~~~~~~~~~~~~~~

#ifndef ECHO_SERVER_MAIN_HPP
#define ECHO_SERVER_MAIN_HPP

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <bas/server.hpp>
#include <bas/io_service_group.hpp>

#include <bastool/server_work.hpp>
#include <bastool/server_work_allocator.hpp>
#include <bastool/server_base.hpp>

#include "config.hpp"
#include "app_param.hpp"

using namespace bas;
using namespace bastool;
using namespace echo;

class server_main
  : public server_base,
    private boost::noncopyable
{
public:
  typedef biz_echo<bgs_none> biz_handler_t;
  typedef server_work<biz_handler_t> server_work_t;
  typedef server_work_allocator<biz_handler_t, bgs_none> server_work_allocator_t;
  typedef client_work<biz_handler_t> client_work_t;
  typedef client_work_allocator<biz_handler_t> client_work_allocator_t;

  typedef server<server_work_t, server_work_allocator_t> server_t;
  typedef service_handler_pool<server_work_t, server_work_allocator_t> server_handler_pool_t;
  typedef service_handler_pool<client_work_t, client_work_allocator_t> client_handler_pool_t;

  typedef boost::shared_ptr<server_t> server_ptr;
  typedef boost::shared_ptr<io_service_group> io_service_group_ptr;

  /// Constructor.
  server_main(const std::string& config_file)
    : config_file_(config_file),
      server_(),
      service_group_()
  {
  }

  /// Destructor.
  ~server_main()
  {
    server_.reset();
    service_group_.reset();
  }

  /// Run the server with block mode.
  void run()
  {
    if (init() == ECHO_ERR_NONE)
    {
      // Start io_service_group with non-blocked mode.
      service_group_->start();

      // Run the server until stopped.
      server_->run();

      // Stop io_service_group.
      service_group_->stop();
    }
  }

  /// Start the server with nonblock mode.
  int start(DWORD argc, LPTSTR *argv)
  {
    int ret = init();

    if (ret != ECHO_ERR_NONE)
      return ret;

    // Start io_service_group with non-blocked mode.
    service_group_->start();

    // Run the server with non-blocked mode.
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
    {
      // Stop server.
      server_->stop();

      // Stop io_service_group.
      service_group_->stop();
    }
  }

private:
  /// Initialize the server objects.
  int init(void)
  {
    using namespace boost::asio::ip;

    if (server_.get() != 0)
      return ECHO_ERR_NONE;

    int ret = get_param(config_file_, param_);
    if (ret != ECHO_ERR_NONE)
      return ret;

    service_group_.reset(new io_service_group(2));
    service_group_->get(io_service_group::io_pool).set(param_.io_thread_size,
        param_.io_thread_size);
    service_group_->get(io_service_group::work_pool).set(param_.work_thread_init,
        param_.work_thread_high,
        param_.work_thread_load);

    server_.reset(new server_t(new server_handler_pool_t(new server_work_allocator_t(0),
                                                         param_.handler_pool_init,
                                                         param_.read_buffer_size,
                                                         param_.write_buffer_size,
                                                         param_.session_timeout,
                                                         param_.io_timeout,
                                                         param_.handler_pool_low,
                                                         param_.handler_pool_high,
                                                         param_.handler_pool_inc,
                                                         param_.handler_pool_max),
                               tcp::endpoint(address::from_string(param_.ip), param_.port),
                               service_group_,
                               param_.accept_queue_size));

    if (server_.get() == 0)
      return ECHO_ERR_ALLOC_FAILED;

    return ECHO_ERR_NONE;
  }

private:
  /// The config file of server.
  std::string config_file_;

  /// The application parameters of server.
  app_param param_;

  /// The pointer of server.
  server_ptr server_; 

  /// The group of io_service_pool objects used to perform asynchronous operations.
  io_service_group_ptr service_group_;
};

#endif // ECHO_SERVER_MAIN_HPP

