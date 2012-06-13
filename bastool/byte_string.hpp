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
#include <bas/io_buffer.hpp>
#include <algorithm>
#include <memory>
#include <vector>

namespace bastool {

#define BASTOOL_BYTE_STRING_DEFAULT_CAPACITY    256

using namespace bas;

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
  byte_string(size_t length, const byte_t* data)
    : buffer_(length, 0)
  {
    BOOST_ASSERT(data != 0);

    memcpy(&buffer_[0], data, length);
  }

  /// Constructor to filling with the specified byte.
  byte_string(size_t length, byte_t byte)
    : buffer_(length, byte)
  {
  }

  /// Copy constructor.
  byte_string(const byte_string& other)
    : buffer_(other.buffer_)
  {
  }

  /// Copy constructor from std::string.
  byte_string(const std::string& other)
    : buffer_(other.size() + 1, 0)
  {
    memcpy(&buffer_[0], (const byte_t*)other.c_str(), other.size() + 1);
  }

  /// Copy constructor from io_buffer.
  byte_string(const io_buffer& other)
    : buffer_(other.size(), 0)
  {
    memcpy(&buffer_[0], other.data(), other.size());
  }

  /// Assign from other byte_string/std::string/io_buffer.
  template<typename T>
  byte_string& operator= (const T& other)
  {
    assign(other);

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

  /// Erases all elements of the buffer.
  void clear()
  {
    buffer_.clear();
  }

  /// Assign the buffer with the specified data.
  void assign(size_t length, const byte_t* data)
  {
    BOOST_ASSERT(data != 0);

    buffer_.resize(length);
    memcpy(&buffer_[0], data, length);
  }

  /// Assign the buffer fill with the specified byte.
  void assign(size_t length, byte_t byte)
  {
    buffer_.resize(length);
    memset(&buffer_[0], byte, length);
  }

  /// Assign the buffer with other byte_string.
  void assign(const byte_string& other)
  {
    buffer_ = other.buffer_;
  }

  /// Assign the buffer with other std::string.
  void assign(const std::string& other)
  {
    assign(other.size() + 1, (const byte_t *)other.c_str());
  }

  /// Assign the buffer with other io_buffer.
  void assign(const io_buffer& other)
  {
    assign(other.size(), other.data());
  }

  /// Append the buffer with the specified data.
  void append(size_t length, const byte_t* data)
  {
    BOOST_ASSERT(data != 0);

    size_t old_size = buffer_.size();
    buffer_.resize(old_size + length);
    memcpy(&buffer_[0] + old_size, data, length);
  }

  /// Append the buffer filling with the specified byte.
  void append(size_t length, byte_t byte)
  {
    size_t old_size = buffer_.size();
    buffer_.resize(old_size + length);
    memset(&buffer_[0] + old_size, byte, length);
  }

  /// Append the buffer with other byte_string/io_buffer.
  template<typename T>
  void append(const T& other)
  {
    append(other.size(), other.data());
  }

  /// Append the buffer with the specified data.
  void append(const std::string& other)
  {
    append(other.size() + 1, (const byte_t *)other.c_str());
  }

  /// Return part of the byte_string.
  byte_string substr(size_t position, size_t length)
  {
    if (position + length > size() || length == 0)
      position = length = 0;

    return byte_string(length, data() + position);
  }

  /// Fill the buffer with the specified byte.
  void fill(byte_t byte)
  {
    memset(&buffer_[0], byte, buffer_.size());
  }

  /// Get hash value.
  const size_t hash_value() const
  {
    return boost::hash_range(buffer_.begin(), buffer_.end());
  }

  /// Append the buffer with other byte_string/std::string/io_buffer.
  template<typename T>
  byte_string& operator+ (const T& rhs)
  {
    this->append(rhs);

    return *this;
  }

  /// Append the buffer with other byte_string/std::string/io_buffer.
  template<typename T>
  byte_string& operator+= (const T& rhs)
  {
    this->append(rhs);

    return *this;
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
