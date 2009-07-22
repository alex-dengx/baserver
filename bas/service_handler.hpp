//
// service_handler.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2009 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BAS_SERVICE_HANDLER_HPP
#define BAS_SERVICE_HANDLER_HPP

#include <boost/assert.hpp>
#include <boost/any.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <bas/io_buffer.hpp>

namespace bas {

/// Struct for deliver event cross multiple hander.
struct event
{
  enum state_t
  {
    none = 0,
    parent_open,
    parent_read,
    parent_write,
    parent_close,
    child_open,
    child_read,
    child_write,
    child_close,
    user = 1000
  };

  std::size_t state_;
  std::size_t value_;

  event(std::size_t state = 0,
      std::size_t value = 0)
    : state_(state),
      value_(value)
  {
  }
};

template<typename, typename, typename> class service_handler_pool;
template<typename, typename, typename> class server;
template<typename, typename, typename> class client;

/// Object for handle socket asynchronous operations.
template<typename Work_Handler, typename Socket_Service = boost::asio::ip::tcp::socket>
class service_handler
  : public boost::enable_shared_from_this<service_handler<Work_Handler, Socket_Service> >,
    private boost::noncopyable
{
public:
  using boost::enable_shared_from_this<service_handler<Work_Handler, Socket_Service> >::shared_from_this;

  /// The type of the service_handler.
  typedef service_handler<Work_Handler, Socket_Service> service_handler_type;

  /// The type of the work_handler.
  typedef Work_Handler work_handler_type;

  /// The type of the socket that will be used to provide asynchronous operations.
  typedef Socket_Service socket_type;

  /// Construct a service_handler object.
  explicit service_handler(work_handler_type* work_handler,
      std::size_t read_buffer_size,
      std::size_t write_buffer_size,
      std::size_t timeout_seconds,
      std::size_t closed_wait_delay)
    : work_handler_(work_handler),
      socket_(),
      timer_(),
      io_service_(0),
      work_service_(0),
      parent_handler_(0),
      child_handler_(0),
      timer_count_(0),
      stopped_(true),
      closed_(true),
      timeout_seconds_(timeout_seconds),
      closed_wait_time_(boost::posix_time::seconds(closed_wait_delay)),
      restriction_time_(boost::posix_time::microsec_clock::universal_time()),
      read_buffer_(),
      write_buffer_()
  {
    BOOST_ASSERT(work_handler != 0);
    BOOST_ASSERT(closed_wait_delay != 0);

    read_buffer_.reset(new io_buffer(read_buffer_size));
    write_buffer_.reset(new io_buffer(write_buffer_size));
  }
  
  /// Destruct the service handler.
  ~service_handler()
  {
    clear_parent_handler();
    clear_child_handler();
  }

  /// Get the io_buffer for incoming data.
  io_buffer& read_buffer()
  {
    BOOST_ASSERT(read_buffer_.get() != 0);
    return *read_buffer_;
  }

  /// Get the io_buffer for outcoming data.
  io_buffer& write_buffer()
  {
    BOOST_ASSERT(write_buffer_.get() != 0);
    return *write_buffer_;
  }

  /// Get the socket associated with the service_handler.
  socket_type& socket()
  {
    BOOST_ASSERT(socket_.get() != 0);
    return *socket_;
  }

  /// Get the io_service object used to perform asynchronous operations.
  boost::asio::io_service& io_service()
  {
    BOOST_ASSERT(io_service_ != 0);
    return *io_service_;
  }

  /// Get the io_service object used to dispatch synchronous works.
  boost::asio::io_service& work_service()
  {
    BOOST_ASSERT(work_service_ != 0);
    return *work_service_;
  }

  /// Get the parent handler.
  boost::any* parent_handler()
  {
    return parent_handler_;
  }

  /// Get the child handler.
  boost::any* child_handler()
  {
    return child_handler_;
  }

  /// Close the handler with error_code e from any thread.
  void close(const boost::system::error_code& e)
  {
    // Dispatch to io_service thread.
    io_service().dispatch(boost::bind(&service_handler_type::close_i,
        shared_from_this(),
        e));
  }

  /// Close the handler with error_code 0 from any thread.
  void close()
  {
    close(boost::system::error_code(0, boost::system::get_system_category()));
  }

  /// Start an asynchronous operation from any thread to read any amount of data from the socket.
  void async_read_some()
  {
    BOOST_ASSERT(read_buffer().space() != 0);

    async_read_some(boost::asio::buffer(read_buffer().data() + read_buffer().size(), read_buffer().space()));
  }

  /// Start an asynchronous operation from any thread to read any amount of data to buffers from the socket.
#if defined(__GNUC__)
  void async_read_some(boost::asio::mutable_buffers_1 buffers)
  {
    io_service().dispatch(boost::bind(&service_handler_type::async_read_some_1,
        shared_from_this(),
        buffers));
  }
#else  // other c++ compiler
  template<typename Buffers>
  void async_read_some(const Buffers& buffers)
  {
    io_service().dispatch(boost::bind(&service_handler_type::async_read_some_i<Buffers>,
        shared_from_this(),
        buffers));
  }
#endif

  /// Start an asynchronous operation from any thread to read a certain amount of data from the socket.
  void async_read(std::size_t length)
  {
    BOOST_ASSERT(length != 0 && length <= read_buffer().space());

    async_read(boost::asio::buffer(read_buffer().data() + read_buffer().size(), length));
  }

  /// Start an asynchronous operation from any thread to read a certain amount of data to buffers from the socket.
#if defined(__GNUC__)
  void async_read(boost::asio::mutable_buffers_1 buffers)
  {
    io_service().dispatch(boost::bind(&service_handler_type::async_read_1,
        shared_from_this(),
        buffers));
  }
#else  // other c++ compiler
  template<typename Buffers>
  void async_read(const Buffers& buffers)
  {
    io_service().dispatch(boost::bind(&service_handler_type::async_read_i<Buffers>,
        shared_from_this(),
        buffers));
  }
#endif

  /// Start an asynchronous operation from any thread to write a certain amount of data to the socket.
  void async_write()
  {
    BOOST_ASSERT(write_buffer().size() != 0);

    async_write(boost::asio::buffer(write_buffer().data(), write_buffer().size()));
  }

  /// Start an asynchronous operation from any thread to write a certain amount of data to the socket.
  void async_write(std::size_t length)
  {
    BOOST_ASSERT(length != 0 && length <= write_buffer().size());

    async_write(boost::asio::buffer(write_buffer().data(), length));
  }

  /// Start an asynchronous operation from any thread to write buffers to the socket.
#if defined(__GNUC__)
  void async_write(boost::asio::mutable_buffers_1 buffers)
  {
    io_service().dispatch(boost::bind(&service_handler_type::async_write_1,
        shared_from_this(),
        buffers));
  }
#else  // other c++ compiler
  template<typename Buffers>
  void async_write(const Buffers& buffers)
  {
    io_service().dispatch(boost::bind(&service_handler_type::async_write_i<Buffers>,
        shared_from_this(),
        buffers));
  }
#endif

  /// Dispatch event from the parent handler.
  void post_parent(const bas::event event)
  {
    if (parent_handler_ != 0)
    {
      work_service().post(boost::bind(&service_handler_type::do_parent,
          shared_from_this(),
          event));
    }
  }

  /// Dispatch event from the child handler.
  void post_child(const bas::event event)
  {
    if (child_handler_ != 0)
    {
      work_service().post(boost::bind(&service_handler_type::do_child,
          shared_from_this(),
          event));
    }
  }

private:
  template<typename, typename, typename> friend class service_handler_pool;
  template<typename, typename, typename> friend class server;
  template<typename, typename, typename> friend class client;
  template<typename, typename> friend class service_handler;

  /// Set the parent handler.
  template<typename Parent_Handler>
  void parent_handler(Parent_Handler* handler)
  {
    clear_parent_handler();

    if (handler != 0)
    {
      parent_handler_ = new boost::any(handler);
    }
  }

  /// Set the child handler.
  template<typename Child_Handler>
  void child_handler(Child_Handler* handler)
  {
    clear_child_handler();

    if (handler != 0)
    {
      child_handler_ = new boost::any(handler);
    }
  }

  /// Clear the parent handler.
  void clear_parent_handler()
  {
    if (parent_handler_ != 0)
    {
      delete parent_handler_;
      parent_handler_ = 0;
    }
  }

  /// Clear the child handler.
  void clear_child_handler()
  {
    if (child_handler_ != 0)
    {
      delete child_handler_;
      child_handler_ = 0;
    }
  }

  /// Check handler is in busy or not.
  bool is_busy()
  {
    return !closed_ || (boost::posix_time::microsec_clock::universal_time() < restriction_time_);
  }

  /// Bind a service_handler with the given io_service and work_service.
  template<typename Work_Allocator>
  void bind(boost::asio::io_service& io_service,
      boost::asio::io_service& work_service,
      Work_Allocator* work_allocator)
  {
    BOOST_ASSERT(work_allocator != 0);
    BOOST_ASSERT(timer_count_ == 0);

    closed_ = false;
    stopped_ = false;

    socket_.reset(work_allocator->make_socket(io_service));
    timer_.reset(new boost::asio::deadline_timer(io_service));

    io_service_ = &socket_->get_io_service();
    work_service_ = &work_service;

    // Clear buffers for new operations.
    read_buffer().clear();
    write_buffer().clear();

    // Clear work handler for new operations.
    work_handler_->on_clear(*this);
  }

  /// Start an asynchronous connect.
  void connect(boost::asio::ip::tcp::endpoint& endpoint)
  {
    BOOST_ASSERT(socket_.get() != 0);
    BOOST_ASSERT(timer_.get() != 0);
    BOOST_ASSERT(work_service_ != 0);

    // Set timer for connection timeout.
    // If reconnect, timer has been setted, don't set again.
    if (timer_count_ == 0)
      set_expiry(timeout_seconds_);

    socket().lowest_layer().async_connect(endpoint,
        boost::bind(&service_handler_type::handle_connect,
            shared_from_this(),
            boost::asio::placeholders::error));
  }

  /// Start an asynchronous connect with the parent handler.
  template<typename Parent_Handler>
  void connect(boost::asio::ip::tcp::endpoint& endpoint,
      Parent_Handler* handler)
  {
    BOOST_ASSERT(handler != 0);

    BOOST_ASSERT(parent_handler_ == 0);
    BOOST_ASSERT(child_handler_ == 0);

    parent_handler(handler);
    handler->child_handler(this);

    connect(endpoint);
  }

  /// Start the first operation.
  void start()
  {
    BOOST_ASSERT(socket_.get() != 0);
    BOOST_ASSERT(timer_.get() != 0);
    BOOST_ASSERT(work_service_ != 0);

    // If start from connect, timer has been setted, don't set again.
    if (timer_count_ == 0)
      set_expiry(timeout_seconds_);

    // Post to work_service for executing do_open.
    work_service().post(boost::bind(&service_handler_type::do_open,
        shared_from_this()));
  }

private:
#if defined(__GNUC__)
  /// Start an asynchronous operation from io_service thread to read any amount of data to buffers from the socket.
  void async_read_some_1(const boost::asio::mutable_buffers_1& buffers)
  {
    async_read_some_i(buffers);
  }
#endif

  /// Start an asynchronous operation from io_service thread to read any amount of data to buffers from the socket.
  template<typename Buffers>
  void async_read_some_i(const Buffers& buffers)
  {
    // The handler has been stopped, do nothing.
    if (stopped_)
      return;

    socket().async_read_some(buffers,
        boost::bind(&service_handler_type::handle_read,
            shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
  }

#if defined(__GNUC__)
  /// Start an asynchronous operation from io_service thread to read a certain amount of data to buffers from the socket.
  void async_read_1(const boost::asio::mutable_buffers_1& buffers)
  {
    async_read_i(buffers);
  }
#endif

  /// Start an asynchronous operation from io_service thread to read a certain amount of data to buffers from the socket.
  template<typename Buffers>
  void async_read_i(const Buffers& buffers)
  {
    // The handler has been stopped, do nothing.
    if (stopped_)
      return;

    boost::asio::async_read(socket(),
        buffers,
        boost::bind(&service_handler_type::handle_read,
            shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
  }

#if defined(__GNUC__)
  /// Start an asynchronous operation from io_service thread to write buffers to the socket.
  void async_write_1(const boost::asio::mutable_buffers_1& buffers)
  {
    async_write_i(buffers);
  }
#endif

  /// Start an asynchronous operation from io_service thread to write buffers to the socket.
  template<typename Buffers>
  void async_write_i(const Buffers& buffers)
  {
    // The handler has been stopped, do nothing.
    if (stopped_)
      return;

    boost::asio::async_write(socket(),
        buffers,
        boost::bind(&service_handler_type::handle_write,
            shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
  }

  /// Set timer for connection.
  void set_expiry(std::size_t timeout_seconds)
  {
    if (timeout_seconds == 0)
      return;

    BOOST_ASSERT(timer_.get() != 0);

    ++timer_count_;
    timer_->expires_from_now(boost::posix_time::seconds(timeout_seconds));
    timer_->async_wait(boost::bind(&service_handler_type::handle_timeout,
        shared_from_this(),
        boost::asio::placeholders::error));
  }

  /// Handle completion of a connect operation in io_service thread.
  void handle_connect(const boost::system::error_code& e)
  {
    // The handler has been stopped, do nothing.
    if (stopped_)
      return;

    if (!e)
    {
      start();
    }
    else
    {
      // Close with error_code e.
      close_i(e);
    }
  }

  /// Handle completion of a read operation in io_service thread.
  void handle_read(const boost::system::error_code& e,
      std::size_t bytes_transferred)
  {
    // The handler has been stopped, do nothing.
    if (stopped_)
      return;

    if (!e)
    {
      // Post to work_service for executing do_read.
      work_service().post(boost::bind(&service_handler_type::do_read,
          shared_from_this(),
          bytes_transferred));
    }
    else
    {
      // Close with error_code e.
      close_i(e);
    }
  }

  /// Handle completion of a write operation in io_service thread.
  void handle_write(const boost::system::error_code& e,
      std::size_t bytes_transferred)
  {
    // The handler has been stopped, do nothing.
    if (stopped_)
      return;

    if (!e)
    {
      // Post to work_service for executing do_write.
      work_service().post(boost::bind(&service_handler_type::do_write,
          shared_from_this(),
          bytes_transferred));
    }
    else
    {
      // Close with error_code e.
      close_i(e);
    }
  }

  /// Handle timeout of whole operation in io_service thread.
  void handle_timeout(const boost::system::error_code& e)
  {
    --timer_count_;

    // The handler has been stopped or timer has been cancelled, do nothing.
    if (stopped_ || e == boost::asio::error::operation_aborted)
      return;

    if (!e)
    {
      // Close with error_code boost::system::timed_out.
      close_i(boost::system::error_code(boost::asio::error::timed_out, boost::system::get_system_category()));
    }
    else
    {
      // Close with error_code e.
      close_i(e);
    }
  }

  /// Close the handler in io_service thread.
  void close_i(const boost::system::error_code& e)
  {
    if (!stopped_)
    {
      stopped_ = true;

      // Initiate graceful service_handler closure.
      boost::system::error_code ignored_ec;
      socket().lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
      socket().lowest_layer().close();

      // Timer has not been expired, or expired but not dispatched,
      // cancel it even if expired.
      if (timer_count_ != 0)
        timer_->cancel();

      // Post to work_service to executing do_close.
      work_service().post(boost::bind(&service_handler_type::do_close,
          shared_from_this(),
          e));
    }
  }

  /// Do on_open in work_service thread.
  void do_open()
  {
    // The handler has been stopped, do nothing.
    if (stopped_)
      return;

    // Call on_open function of the work handler.
    work_handler_->on_open(*this);
  }

  /// Do on_read in work_service thread.
  void do_read(std::size_t bytes_transferred)
  {
    // The handler has been stopped, do nothing.
    if (stopped_)
      return;

    // Call on_read function of the work handler.
    work_handler_->on_read(*this, bytes_transferred);
  }

  /// Do on_write in work_service thread.
  void do_write(std::size_t bytes_transferred)
  {
    // The handler has been stopped, do nothing.
    if (stopped_)
      return;

    // Call on_write function of the work handler.
    work_handler_->on_write(*this, bytes_transferred);
  }

  /// Do on_parent in work_service thread.
  void do_parent(const bas::event event)
  {
    // The handler has been stopped, do nothing.
    if (stopped_)
      return;

    // Call on_parent function of the work handler.
    work_handler_->on_parent(*this, event);
  }

  /// Do on_child in work_service thread.
  void do_child(const bas::event event)
  {
    // The handler has been stopped, do nothing.
    if (stopped_)
      return;

    // Call on_child function of the work handler.
    work_handler_->on_child(*this, event);
  }

  /// Do on_close and reset handler for next connaction in work_service thread.
  void do_close(const boost::system::error_code& e)
  {
    // Call on_close function of the work handler.
    work_handler_->on_close(*this, e);

    // Clear parent handler and child handler.
    clear_parent_handler();
    clear_child_handler();

    timer_.reset();

    // Set restriction time before reuse, wait uncompleted operations to be finished.
    restriction_time_ = boost::posix_time::microsec_clock::universal_time() + closed_wait_time_;

    closed_ = true;

    // Leave socket to destroy delay for finishing uncompleted SSL operations.
    // Leave read_buffer_/write_buffer_/io_service_/work_service_ for finishing uncompleted operations.
  }

private:
  typedef boost::shared_ptr<socket_type> socket_ptr;
  typedef boost::shared_ptr<work_handler_type> work_handler_ptr;
  typedef boost::shared_ptr<boost::asio::deadline_timer> timer_ptr;
  typedef boost::shared_ptr<io_buffer> io_buffer_ptr;

  /// Work handler of the service_handler.
  work_handler_ptr work_handler_;

  /// Socket for the service_handler.
  socket_ptr socket_;

  /// Timer for timeout operation.
  timer_ptr timer_;

  /// The io_service for for asynchronous operations.
  boost::asio::io_service* io_service_;

  /// The io_service for for executing synchronous works.
  boost::asio::io_service* work_service_;

  /// Parent handler of the handler.
  boost::any* parent_handler_;
  
  /// Child handler of the handler.
  boost::any* child_handler_;

  /// Count of waiting timer.
  std::size_t timer_count_;

  // Flag to indicate that the handler has been stopped and can not do synchronous operations.
  bool stopped_;

  // Flag to indicate that the handler has been closed and can be reuse again after some seconds.
  bool closed_;

  /// The expiry seconds of connection.
  std::size_t timeout_seconds_;

  /// For graceful close, if preallocated service_handler number is not huge enough,
  /// should delay some seconds before any handler reuse again.
  boost::posix_time::time_duration closed_wait_time_;

  /// The time of hander can use again.
  boost::posix_time::ptime restriction_time_;

  /// Buffer for incoming data.
  io_buffer_ptr read_buffer_;

  /// Buffer for outcoming data.
  io_buffer_ptr write_buffer_;
};

} // namespace bas

#endif // BAS_SERVICE_HANDLER_HPP
