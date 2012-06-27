//
// io_buffer.hpp
// ~~~~~~~~~~~~~
//
// The class based on boost::asio::detail::buffered_stream_storage.
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Copyright (c) 2009 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BAS_IO_BUFFER_HPP
#define BAS_IO_BUFFER_HPP

#include <boost/assert.hpp>
#include <memory>
#include <vector>

namespace bas {

/// Buffer for incoming and outcoming data.
class io_buffer
{
public:
  /// The type of the bytes stored in io_buffer.
  typedef unsigned char byte_t;

  /// Define type reference of std::size_t.
  typedef std::size_t size_t;

  /// Default constructor.
  io_buffer(size_t capacity)
    : buffer_(capacity, 0),
      begin_offset_(0),
      end_offset_(0)
  {
  }

  /// Constructor with the specified data.
  io_buffer(size_t length, byte_t* data)
    : buffer_(length, 0),
      begin_offset_(0),
      end_offset_(length)
  {
    BOOST_ASSERT(data != 0);

    memcpy(&buffer_[0], data, length);
  }

  /// Copy constructor.
  io_buffer(const io_buffer& other)
    : buffer_(other.buffer_),
      begin_offset_(other.begin_offset_),
      end_offset_(other.end_offset_)
  {
  }

  /// Assign from another.
  io_buffer& operator= (const io_buffer& other)
  {
    buffer_ = other.buffer_;
    begin_offset_ = other.begin_offset_;
    end_offset_ = other.end_offset_;

    return *this;
  }

  /// Clear the buffer.
  void clear()
  {
    begin_offset_ = 0;
    end_offset_ = 0;
  }

  /// Return a pointer to the beginning of the unread data.
  byte_t* data()
  {
    return &buffer_[0] + begin_offset_;
  }

  /// Return a pointer to the beginning of the unread data.
  const byte_t* data() const
  {
    return &buffer_[0] + begin_offset_;
  }

  /// Is there no unread data in the buffer.
  bool empty() const
  {
    return begin_offset_ == end_offset_;
  }

  /// Return the amount of unread data in the buffer.
  const size_t size() const
  {
    return end_offset_ - begin_offset_;
  }

  /// Resize the buffer to the specified length.
  void resize(size_t length)
  {
    BOOST_ASSERT(length <= capacity());

    if (begin_offset_ + length <= capacity())
      end_offset_ = begin_offset_ + length;
    else
    {
      memmove(&buffer_[0], &buffer_[0] + begin_offset_, size());
      end_offset_ = length;
      begin_offset_ = 0;
    }
  }

  /// Return the maximum size for data in the buffer.
  size_t capacity() const
  {
    return buffer_.size();
  }

  /// Return the amount of free space in the buffer.
  const size_t space() const
  {
    return capacity() - end_offset_;
  }

  /// Consume multiple bytes from the beginning of the buffer.
  void consume(size_t count)
  {
    BOOST_ASSERT(count <= size());

    begin_offset_ += count;
    if (empty())
      clear();
  }

  /// Produce multiple bytes to the ending of the buffer.
  void produce(size_t count)
  {
    BOOST_ASSERT(count <= space());

    end_offset_ += count;
  }

  /// Produce data to the ending of the buffer.
  void produce(size_t length, const byte_t* data)
  {
    BOOST_ASSERT(length <= space());

    memcpy(&buffer_[0] + end_offset_, data, length);
    end_offset_ += length;
  }

  /// Produce other buffer to the ending of the buffer.
  template<typename Buffer>
  void produce(const Buffer& other)
  {
    produce(other.size(), other.data());
  }

  /// Remove consumed bytes from the beginning of the buffer.
  void crunch()
  {
    if (begin_offset_ != 0)
    {
      if (empty())
        clear();
      else
      {
        memmove(&buffer_[0], &buffer_[0] + begin_offset_, size());
        end_offset_ = size();
        begin_offset_ = 0;
      }
    }
  }

private:
  /// The offset to the beginning of the unread data.
  size_t begin_offset_;

  /// The offset to the end of the unread data.
  size_t end_offset_;
  
  /// The data in the buffer.
  std::vector<byte_t> buffer_;
};

} // namespace bas

#endif // BAS_IO_BUFFER_HPP
