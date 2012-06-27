//
// sync_handler.hpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2012 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BAS_SYNC_HANDLER_HPP
#define BAS_SYNC_HANDLER_HPP

#include <boost/assert.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include <bas/io_buffer.hpp>

namespace bas {

/// Object for handle socket synchronous operations.
template<typename Socket_Service = boost::asio::ip::tcp::socket>
class sync_handler
  : public boost::enable_shared_from_this<sync_handler<Socket_Service> >,
    private boost::noncopyable
{
public:
  using boost::enable_shared_from_this<sync_handler<Socket_Service> >::shared_from_this;

  /// Define type reference of std::size_t.
  typedef std::size_t size_t;

  /// The type of the socket that will be used to provide asynchronous operations.
  typedef Socket_Service socket_t;

  /// Define type reference of boost::asio::io_service.
  typedef boost::asio::io_service io_service_t;

  /// Define type reference of boost::asio::deadline_timer.
  typedef boost::asio::deadline_timer timer_t;

  /// Define type reference of boost::asio::ip::tcp::endpoint.
  typedef boost::asio::ip::tcp::endpoint endpoint_t;

  /// Define type reference of boost::recursive_mutex.
  typedef boost::recursive_mutex mutex_t;

  /// Define type reference of boost::recursive_mutex::scoped_lock.
  typedef boost::recursive_mutex::scoped_lock scoped_lock_t;

  /// Define type reference of boost::system::error_code.
  typedef boost::system::error_code error_t;

  /// Define the type of the sync_handler.
  typedef sync_handler<socket_t> sync_handler_t;

  /// Constructor.
  sync_handler(io_service_t& io_service,
      endpoint_t& peer_endpoint,
      endpoint_t& local_endpoint,
      size_t buffer_size,
      long timeout_milliseconds)
    : io_service_(io_service),
      socket_(io_service),
      timer_(io_service),
      peer_endpoint_(peer_endpoint),
      local_endpoint_(local_endpoint),
      buffer_(buffer_size),
      timeout_milliseconds_(timeout_milliseconds),
      mutex_(),
      condition_(),
      ec_(boost::asio::error::shut_down),
      bytes_transferred_(0),
      opened_(false),
      duplex_(false),
      pending_(false),
      waiting_(false)
  {
    BOOST_ASSERT(timeout_milliseconds_ != 0);
  }

  /// Destructor.
  ~sync_handler()
  {
    clear();
  }

  /// Get the internal io_buffer.
  io_buffer& buffer()
  {
    return buffer_;
  }

  /// Get the io_service object used to perform asynchronous operations.
  io_service_t& io_service()
  {
    return io_service_;
  }

  /// Get the socket associated with the handler.
  socket_t& socket()
  {
    return socket_;
  }

  /// Get the peer_endpoint associated with the handler.
  endpoint_t& peer_endpoint()
  {
    return peer_endpoint_;
  }

  /// Get the local_endpoint associated with the handler.
  endpoint_t& local_endpoint()
  {
    return local_endpoint_;
  }

  /// Close the handler.
  void close()
  {
    // Post to io_service thread.
    io_service_.post(boost::bind(&sync_handler_t::close_i,
                                 shared_from_this()));
  }

  /// Get the error code of the handler.
  error_t error_code()
  {
    // Lock for synchronize access to data.
    scoped_lock_t lock(mutex_);

    // Return error if another operation already started.
    if (waiting_)
      return error_t(boost::asio::error::already_started);

    return ec_; 
  }

  /// Establish a connection with the internal endpoint.
  error_t connect(bool reconnect = false)
  {
    return connect(peer_endpoint_, local_endpoint_, reconnect);
  }

  /// Establish a connection with the given endpoint.
  error_t connect(endpoint_t& peer_endpoint,
              endpoint_t& local_endpoint = endpoint_t(),
              bool reconnect = false)
  {
    // Lock for synchronize access to data.
    scoped_lock_t lock(mutex_);

    // Return error if another operation already started.
    if (waiting_)
      return error_t(boost::asio::error::already_started);

    // Return if already opened.
    if (!ec_ && opened_ && !reconnect)
      return ec_;

    // Post to io_service thread.
    io_service_.post(boost::bind(&sync_handler_t::connect_i,
                                 shared_from_this(),
                                 peer_endpoint,
                                 local_endpoint,
                                 reconnect));

    // Waiting for notify.
    waiting_ = true;
    condition_.wait(lock);
    waiting_ = false;

    return ec_;
  }

  /// Read some data to the default buffer.
  error_t read_some(size_t& bytes_transferred)
  {
    bytes_transferred = 0;

    if (buffer().space() == 0)
      return error_t(boost::asio::error::invalid_argument);
    else
      return read_some(boost::asio::buffer(buffer().data() + buffer().size(), buffer().space()),
                 bytes_transferred);
  }

  /// Read some data to the given buffer.
  template<typename Buffers>
  error_t read_some(const Buffers& buffers, size_t& bytes_transferred)
  {
    bytes_transferred = 0;

    // Lock for synchronize access to data.
    scoped_lock_t lock(mutex_);

    // Return error if another operation already started.
    if (waiting_)
      return error_t(boost::asio::error::already_started);

    // Post to io_service thread.
    io_service_.post(boost::bind(&sync_handler_t::read_some_i<Buffers>,
                                 shared_from_this(),
                                 buffers));

    // Waiting for notify.
    waiting_ = true;
    condition_.wait(lock);
    waiting_ = false;

    bytes_transferred = bytes_transferred_;
    return ec_;
  }

  /// Read data to the default buffer.
  error_t read(size_t& bytes_transferred)
  {
    bytes_transferred = 0;

    if (buffer().space() == 0)
      return error_t(boost::asio::error::invalid_argument);
    else
      return read(boost::asio::buffer(buffer().data() + buffer().size(), buffer().space()),
                  bytes_transferred);
  }

  /// Read data to the default buffer.
  error_t read(size_t length, size_t& bytes_transferred)
  {
    bytes_transferred = 0;

    if (length == 0 || length > buffer().space())
      return error_t(boost::asio::error::invalid_argument);
    else
      return read(boost::asio::buffer(buffer().data() + buffer().size(), length),
                  bytes_transferred);
  }

  /// Read data to the given buffer.
  template<typename Buffers>
  error_t read(const Buffers& buffers, size_t& bytes_transferred)
  {
    bytes_transferred = 0;

    // Lock for synchronize access to data.
    scoped_lock_t lock(mutex_);

    // Return error if another operation already started.
    if (waiting_)
      return error_t(boost::asio::error::already_started);

    // Post to io_service thread.
    io_service_.post(boost::bind(&sync_handler_t::read_i<Buffers>,
                                 shared_from_this(),
                                 buffers));

    // Waiting for notify.
    waiting_ = true;
    condition_.wait(lock);
    waiting_ = false;

    bytes_transferred = bytes_transferred_;
    return ec_;
  }

  /// Write the default buffer.
  error_t write(size_t& bytes_transferred)
  {
    bytes_transferred = 0;

    if (buffer().empty())
      return error_t(boost::asio::error::invalid_argument);
    else
      return write(boost::asio::buffer(buffer().data(), buffer().size()),
                 bytes_transferred);
  }

  /// Write the default buffer.
  void write(size_t length, size_t& bytes_transferred)
  {
    bytes_transferred = 0;

    if (length == 0 || length > buffer().size())
      return error_t(boost::asio::error::invalid_argument);
    else
      return write(boost::asio::buffer(buffer().data(), length),
                 bytes_transferred);
  }

  /// Write the buffer.
  template<typename Buffers>
  error_t write(const Buffers& buffers, size_t& bytes_transferred)
  {
    bytes_transferred = 0;

    // Lock for synchronize access to data.
    scoped_lock_t lock(mutex_);

    // Return error if another operation already started.
    if (waiting_)
      return error_t(boost::asio::error::already_started);

    // Post to io_service thread.
    io_service_.post(boost::bind(&sync_handler_t::write_i<Buffers>,
                                 shared_from_this(),
                                 buffers));

    // Waiting for notify.
    waiting_ = true;
    condition_.wait(lock);
    waiting_ = false;

    bytes_transferred = bytes_transferred_;
    return ec_;
  }

  /// Write to and then read from the socket.
  error_t write_read(size_t& bytes_transferred)
  {
    bytes_transferred = 0;

    if (buffer().empty())
      return error_t(boost::asio::error::invalid_argument);

    // Lock for synchronize access to data.
    scoped_lock_t lock(mutex_);

    // Return error if another operation already started.
    if (waiting_)
      return error_t(boost::asio::error::already_started);

    // Post to io_service thread.
    io_service_.post(boost::bind(&sync_handler_t::write_read_i,
                                 shared_from_this()));

    // Waiting for notify.
    waiting_ = true;
    condition_.wait(lock);
    waiting_ = false;

    bytes_transferred = bytes_transferred_;
    return ec_;
  }

private:
  template<typename> friend class sync_handler_pool;

  /// Release allocated resources.
  void clear()
  {
    close_i();
  }

  // Notify for operation completed or aborted.
  void notify_i()
  {
    // Lock for synchronize access to data.
    scoped_lock_t lock(mutex_);

    condition_.notify_one();
  }

  /// Start asynchronous connect operation in io_service thread.
  void connect_i(endpoint_t& peer_endpoint,
           endpoint_t& local_endpoint,
           bool reconnect)
  {
    // If the handler is opened and not reconnect, abort operation.
    if (opened_)
    {
      if (!reconnect)
      {
        // Notify for operation completed.
        notify_i();
        return;
      }

      // The handler need reconnect, close current socket.
      close_socket();
    }

    if (local_endpoint != endpoint_t())
    {
      // Open and bind the socket to the given local endpoint.
      socket_.open(local_endpoint.protocol(), ec_);
      if (!ec_)
        socket_.bind(local_endpoint, ec_);

      if (ec_)
      {
        close_socket();

        // Notify for operation aborted.
        notify_i();
        return;
      }
    }

    // Set timer for timeout control and start async_connect.
    set_timer();
    socket_.async_connect(peer_endpoint,
                boost::bind(&sync_handler_t::handle_connect,
                            shared_from_this(),
                            boost::asio::placeholders::error));
  }

  /// Start asynchronous read operation in io_service thread.
  template<typename Buffers>
  void read_some_i(const Buffers& buffers)
  {
    // If the handler is closed, abort operation.
    if (!opened_)
    {
      // Notify for operation aborted.
      notify_i();
      return;
    }

    // Set to indicate read only.
    duplex_ = false;

    // Set timer for timeout control and start async_read_some;
    set_timer();
    socket_.async_read_some(buffers,
                boost::bind(&sync_handler_t::handle_read_write,
                            shared_from_this(),
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
  }

  /// Start asynchronous read operation in io_service thread.
  template<typename Buffers>
  void read_i(const Buffers& buffers)
  {
    // If the handler is closed, abort operation.
    if (!opened_)
    {
      // Notify for operation aborted.
      notify_i();
      return;
    }

    // Set to indicate read only.
    duplex_ = false;

    // Set timer for timeout control and start async_read;
    set_timer();
    boost::asio::async_read(socket_,
        buffers,
        boost::bind(&sync_handler_t::handle_read_write,
                    shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
  }
 
  /// Start asynchronous write operation in io_service thread.
  template<typename Buffers>
  void write_i(const Buffers& buffers)
  {
    // If the handler is closed, abort operation.
    if (!opened_)
    {
      // Notify for operation aborted.
      notify_i();
      return;
    }

    // Set to indicate write only.
    duplex_ = false;

    // Set timer for timeout control and start async_write;
    set_timer();
    boost::asio::async_write(socket_,
        buffers,
        boost::bind(&sync_handler_t::handle_read_write,
                    shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
  }

  /// Start asynchronous write operation in io_service thread.
  void write_read_i()
  {
    // If the handler is closed, abort operation.
    if (!opened_)
    {
      // Notify for operation aborted.
      notify_i();
      return;
    }

    // Set to indicate write first then read next.
    duplex_ = true;

    // Set timer for timeout control and start async_write;
    set_timer();
    boost::asio::async_write(socket_,
        boost::asio::buffer(buffer().data(), buffer().size()),
        boost::bind(&sync_handler_t::handle_read_write,
                    shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
  }

  /// Set timer for asynchronous operation timeout control.
  void set_timer()
  {
    // Clear error code and transferred status.
    ec_.clear();
    bytes_transferred_ = 0;

    // Set to indicate asynchronous operation was started.
    pending_ = true;

    // Set timer to expires from the given milliseconds.
    timer_.expires_from_now(boost::posix_time::milliseconds(timeout_milliseconds_));
    timer_.async_wait(boost::bind(&sync_handler_t::handle_timeout,
                                   shared_from_this(),
                                   boost::asio::placeholders::error));
  }

  /// Cancel timer for asynchronous operation to be completed or aborted.
  void clear_timer()
  {
    // Set to indicate asynchronous operation was stopped.
    pending_ = false;

    // Cancel timer.
    timer_.cancel();
  }

  /// Handle completion of connect operation in io_service thread.
  void handle_connect(const error_t& ec)
  {
    // If ec_ is set when timeout_milliseconds or close, already notified, do nothing.
    //    ec_ should be boost::system::timed_out or boost::asio::error::shut_down.
    if (ec_)
      return;

    // Clear timeout control.
    clear_timer();

    // If operation failed, close the socket.
    if (ec)
      close_socket();

    // Set operation result.
    opened_ = !ec;
    ec_ = ec;

    // Notify for operation completed.
    notify_i();
  }

  /// Handle completion of read/write operation in io_service thread.
  void handle_read_write(const error_t& ec,
      size_t bytes_transferred)
  {
    // If ec_ is set when timeout_milliseconds or close, already notified, do nothing.
    //    ec_ should be boost::system::timed_out or boost::asio::error::shut_down.
    if (!opened_ || ec_)
      return;

    // Clear timeout control.
    clear_timer();

    // If operation failed, close the socket.
    if (ec)
      close_socket();

    if (duplex_ && !ec && bytes_transferred == buffer().size())
    {
      // Read data from the socket.
      buffer().clear();
      read_some_i(boost::asio::buffer(buffer().data(), buffer().space()));

      return;
    }

    // Set operation result.
    if (duplex_)
    {
      // Write operation failed, read operation aborted.
      bytes_transferred_ = 0;
      ec_ = (ec) ? ec : boost::asio::error::operation_aborted;
    }
    else
    {
      bytes_transferred_ = bytes_transferred;
      ec_ = ec;
    }

    // Notify for operation completed.
    notify_i();
  }

  /// Handle timeout_milliseconds in io_service thread.
  void handle_timeout(const error_t& ec)
  {
    // If the asynchronous operation to be completed or closed, already notified, do nothing.
    //   ec_ is set when complete or close, pending_ is cleared when complete.
    //   ec is set to boost::asio::error::operation_aborted when complete.
    if (ec_ || !pending_ || ec == boost::asio::error::operation_aborted)
      return;

    // Close the socket.
    close_socket();

    // Set operation result.
    ec_ = boost::asio::error::timed_out;

    // Notify for operation completed.
    notify_i();
  }

  /// Close the handler in io_service thread.
  void close_i()
  {
    // If opening or opened.
    if (pending_ || opened_)
    {
      // Cancel timer.
      clear_timer();

      // Close the socket.
      close_socket();

      // Initialize status value.
      bytes_transferred_ = 0;
      ec_ = boost::asio::error::shut_down;

      // Notify for operation completed.
      notify_i();
    }
    else
    {
      // Initialize status value.
      bytes_transferred_ = 0;
      ec_ = boost::asio::error::shut_down;
    }
  }

  // Close the socket gracefully.
  void close_socket()
  {
    error_t ignored_ec;
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    socket_.close(ignored_ec);
    opened_ = false;
  }

private:
  /// The io_service for for asynchronous operations.
  io_service_t& io_service_;

  /// The socket for the handler.
  socket_t socket_;

  /// The timer for asynchronous operation timeout control.
  timer_t timer_;

  /// The client endpoint.
  endpoint_t local_endpoint_;

  /// The peer endpoint.
  endpoint_t peer_endpoint_;

  /// The timeout_milliseconds value in milliseconds.
  long timeout_milliseconds_;

  /// Internal buffer for incoming and outcoming data.
  io_buffer buffer_;

  /// Mutex for synchronize access to data.
  mutex_t mutex_;

  /// Condition for notify operation status.
  boost::condition condition_;

  /// Flag to indicate condition is waiting.
  bool waiting_;

  /// Flag to indicate socket is opened.
  bool opened_;

  /// Flag to indicate write to and then read from the socket.
  bool duplex_;

  /// Flag indicate asynchronous operation status:
  ///   ec_:
  ///     when asynchronous operation start, clear ec_.
  ///     when asynchronous operation complete, set ec_ to error occurred.
  ///     when asynchronous operation abort by close, set ec_ to shutdown.
  ///   pending_:
  ///     when asynchronous operation start, set pending_.
  ///     when asynchronous operation complete or abort, clear pending_.

  /// The error code when error occurred.
  error_t ec_;

  /// Flag to indicate asynchronous operation is pending.
  bool pending_;

  /// Number of bytes transferred in the asynchronous operation.
  size_t bytes_transferred_;
};

} // namespace bas

#endif // BAS_SYNC_HANDLER_HPP
