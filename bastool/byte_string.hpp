//
// byte_string.hpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2009, 2011 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BASTOOL_BYTE_STRING_HPP
#define BASTOOL_BYTE_STRING_HPP

#include <boost/assert.hpp>
#include <boost/swap.hpp>
#include <boost/functional/hash.hpp>
#include <algorithm>
#include <memory>
#include <vector>

namespace bastool {

#define BASTOOL_BYTE_STRING_DEFAULT_CAPACITY    256

/// Class for holding and processing unsigned char array.
class byte_string
{
public:
  /// The type of the bytes stored in the buffer.
  typedef unsigned char byte_t;

  /// Define type reference of std::size_t.
  typedef std::size_t size_t;

  /// Default constructor.
  byte_string(size_t capacity = BASTOOL_BYTE_STRING_DEFAULT_CAPACITY)
    : buffer_()
  {
    buffer_.reserve(capacity);
  }

  /// Constructor with the specified data.
  byte_string(size_t length, byte_t* data)
    : buffer_(length, 0)
  {
    BOOST_ASSERT(data != 0);

    memcpy(&buffer_[0], data, length);
  }

  /// Copy constructor.
  byte_string(const byte_string& other)
    : buffer_(other.buffer_)
  {
  }

  /// Assign from another.
  byte_string& operator= (const byte_string& other)
  {
    buffer_ = other.buffer_;

    return *this;
  }

  /// Return a pointer to the data.
  byte_t* data()
  {
    return &buffer_[0];
  }

  /// Return a pointer to the data.
  const byte_t* data() const
  {
    return &buffer_[0];
  }

  /// Returns whether the buffer is empty.
  bool empty() const
  {
    return buffer_.empty();
  }

  /// Return the amount of data in the buffer.
  size_t size() const
  {
    return buffer_.size();
  }
  
  /// Returns the size of the allocated storage space for data in the buffer.
  size_t capacity() const
  {
    return buffer_.capacity();
  }

  /// Resize the buffer to the specified data.
  void resize(size_t length, byte_t* data)
  {
    BOOST_ASSERT(data != 0);

    buffer_.resize(length);
    memcpy(&buffer_[0], data, length);
  }

  /// Get hash value.
  size_t hash_value()
  {
    return boost::hash_range(buffer_.begin(), buffer_.end());
  }

  /// Compare for equality.
  bool operator== (const byte_string& rhs)
  {
    return (buffer_ == rhs.buffer_);
  }

  /// Compare for ordering.
  bool operator< (const byte_string& rhs)
  {
    return (buffer_ < rhs.buffer_);
  }

  /// Compare for inequality.
  bool operator!= (const byte_string& rhs)
  {
    return (buffer_ != rhs.buffer_);
  }

  /// Compare for ordering.
  bool operator> (const byte_string& rhs)
  {
    return (buffer_ > rhs.buffer_);
  }

  /// Compare for ordering.
  bool operator<= (const byte_string& rhs)
  {
    return (buffer_ <= rhs.buffer_);
  }

  /// Compare for ordering.
  bool operator>= (const byte_string& rhs)
  {
    return (buffer_ >= rhs.buffer_);
  }

  /// Global swap()
  void swap(byte_string& rhs)
  {
    buffer_.swap(rhs.buffer_);
  }

private:
  /// The data in the buffer.
  std::vector<byte_t> buffer_;
};

} // namespace bastool

#endif // BASTOOL_BYTE_STRING_HPP
