//
// error_count.hpp
// ~~~~~~~~~~~~~~~

#ifndef BAS_ECHO_ERROR_COUNT_HPP
#define BAS_ECHO_ERROR_COUNT_HPP

#include <boost/asio/detail/mutex.hpp>

namespace echo {

class error_count
{
public:
  /// Define type reference of std::size_t.
  typedef std::size_t size_type;

  error_count()
    : mutex_(),
      timeout_count_(0),
      error_count_(0)
  {
  }

  void reset()
  {
    // Need lock in multiple thread model.
    boost::asio::detail::mutex::scoped_lock lock(mutex_);
    timeout_count_ = 0;
    error_count_ = 0;
  }

  void timeout()
  {
    // Need lock in multiple thread model.
    boost::asio::detail::mutex::scoped_lock lock(mutex_);
    ++timeout_count_;
  }

  size_type get_timeout()
  {
    // Need lock in multiple thread model.
    boost::asio::detail::mutex::scoped_lock lock(mutex_);
    return timeout_count_;
  }

  void error()
  {
    // Need lock in multiple thread model.
    boost::asio::detail::mutex::scoped_lock lock(mutex_);
    ++error_count_;
  }

  size_type get_error()
  {
    // Need lock in multiple thread model.
    boost::asio::detail::mutex::scoped_lock lock(mutex_);
    return error_count_;
  }

private:
  /// Mutex to protect access to internal data.
  boost::asio::detail::mutex mutex_;

  /// Count of error.
  std::size_t timeout_count_;
  std::size_t error_count_;
};

} // namespace echo

#endif // BAS_ECHO_ERROR_COUNT_HPP
