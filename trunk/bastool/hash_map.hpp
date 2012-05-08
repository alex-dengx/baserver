//
// hash_map.hpp
// ~~~~~~~~~~~~
//
// The class based on boost::asio::detail::hash_map.
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Copyright (c) 2009, 2011 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BASTOOL_HASH_MAP_HPP
#define BASTOOL_HASH_MAP_HPP

#include <boost/functional/hash.hpp>
#include <boost/thread.hpp>
#include <boost/noncopyable.hpp>
#include <bastool/byte_string.hpp>
#include <vector>

namespace bastool {

#define BAS_HASH_MAP_DEFAULT_BUCKETS    12289
#define BAS_HASH_MAP_DEFAULT_MUTEXS     769

/// Define hash table sizes.
static std::size_t HASH_TABLE_SIZES[] = {
  3, 13, 23, 53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593,
  49157, 98317, 196613, 393241, 786433, 1572869, 3145739, 6291469,
  12582917, 25165843
};

/// Note: assumes K and V are POD types.
template <typename K, typename V>
class hash_map
  : private boost::noncopyable
{
public:
  /// Define type reference of std::size_t.
  typedef std::size_t size_t;

  /// The type of value in the map.
  typedef std::pair<K, V> value_type;

  /// The type of bucket in the map.
  typedef typename std::vector<value_type> bucket_type;
  
  /// The type of mutex in the map.
  typedef typename boost::shared_mutex mutex_type;

  /// The type of lock in the map.
  typedef boost::shared_lock<mutex_type> read_lock_type;
  typedef boost::unique_lock<mutex_type> write_lock_type; 

  /// Constructor with default number of elements.
  hash_map()
    : num_buckets_(hash_size(BAS_HASH_MAP_DEFAULT_BUCKETS)),
      num_mutexs_(hash_size(BAS_HASH_MAP_DEFAULT_MUTEXS)),
      mutexs_(NULL)
  {
    init();
  }

  /// Constructor with given number of elements.
  hash_map(const size_t num_elements,
           const size_t num_mutexs = BAS_HASH_MAP_DEFAULT_MUTEXS)
    : num_buckets_(hash_size(num_elements)),
      num_mutexs_(hash_size(num_mutexs)),
      mutexs_(NULL)
  {
    init();
  }

  /// Destruct the map.
  ~hash_map()
  {
    clear();
  }

  /// Find an entry in the map.
  V* find(const K& k)
  {
    if (mutexs_ == NULL)
      return NULL;

    size_t bucket_seat = calculate_hash_value(k) % num_buckets_;
    bucket_type& bucket = buckets_[bucket_seat];

    // Use read lock for share read.
    read_lock_type lock(mutexs_[bucket_seat % num_mutexs_]);

    size_t bucket_size = bucket.size();
    for (size_t i = 0; i < bucket_size; ++i)
    {
      if (bucket[i].first == k)
        return &bucket[i].second;
    }

    return NULL;
  }

  /// Find an entry in the map.
  bool find(const K& k, V& v)
  {
    V* p = find(k);

    if (p)
    {
      v = *p;
      return true;
    }
    else
      return false;
  }

  /// Insert a new entry into the map.
  bool insert(const value_type& value)
  {
    if (mutexs_ == NULL)
      return false;

    size_t bucket_seat = calculate_hash_value(value.first) % num_buckets_;
    bucket_type& bucket = buckets_[bucket_seat];

    // Use write lock for exclusive write.
    write_lock_type lock(mutexs_[bucket_seat % num_mutexs_]);

    size_t bucket_size = bucket.size();
    for (size_t i = 0; i < bucket_size; ++i)
    {
      if (bucket[i].first == value.first)
        return false;
    }

    bucket.push_back(value);

    return true;
  }

  /// Insert a new entry into the map.
  bool insert(const K& k, const V& v)
  {
    value_type value(k, v);

    return insert(value);
  }

  /// Update an entry into the map.
  bool update(const value_type& value)
  {
    if (mutexs_ == NULL)
      return false;

    size_t bucket_seat = calculate_hash_value(value.first) % num_buckets_;
    bucket_type& bucket = buckets_[bucket_seat];

    // Use write lock for exclusive write.
    write_lock_type lock(mutexs_[bucket_seat % num_mutexs_]);

    size_t bucket_size = bucket.size();
    for (size_t i = 0; i < bucket_size; ++i)
    {
      if (bucket[i].first == value.first)
      {
        bucket[i].second = value.second;

        return true;
      }
    }

    return false;
  }

  /// Update an entry into the map.
  bool update(const K& k, const V& v)
  {
    value_type value(k, v);    
    
    return update(value);
  }

  /// Insert or update an entry into the map.
  bool insert_update(const value_type& value)
  {
    if (mutexs_ == NULL)
      return false;

    size_t bucket_seat = calculate_hash_value(value.first) % num_buckets_;
    bucket_type& bucket = buckets_[bucket_seat];

    // Use write lock for exclusive write.
    write_lock_type lock(mutexs_[bucket_seat % num_mutexs_]);

    size_t bucket_size = bucket.size();
    for (size_t i = 0; i < bucket_size; ++i)
    {
      if (bucket[i].first == value.first)
      {
        bucket[i].second = value.second;

        return true;
      }
    }

    bucket.push_back(value);

    return true;
  }

  /// Insert or update an entry into the map.
  bool insert_update(const K& k, const V& v)
  {
    value_type value(k, v);    

    return insert_update(value);
  }

  /// Erase an entry from the map.
  bool erase(const K& k)
  {
    if (mutexs_ == NULL)
      return false;

    size_t bucket_seat = calculate_hash_value(k) % num_buckets_;
    bucket_type& bucket = buckets_[bucket_seat];

    // Use write lock for exclusive write.
    write_lock_type lock(mutexs_[bucket_seat % num_mutexs_]);

    size_t bucket_size = bucket.size();
    for (size_t i = 0; i < bucket_size; ++i)
    {
      if (bucket[i].first == k)
      {
        if (i != bucket_size - 1)
          bucket[i] = bucket[bucket_size - 1];

        bucket.pop_back();

        return true;
      }
    }

    return false;
  }

  /// Clear all buckets to empty.
  void reset()
  {
    if ((mutexs_ == NULL) || (buckets_.size() != num_buckets_))
      return;

    for (size_t i = 0; i < num_buckets_; ++i)
    {
      // Use write lock for exclusive write.
      write_lock_type lock(mutexs_[i % num_mutexs_]);
      
      buckets_[i].clear();
    }
  }

  /// Clean invalid entrys in the map.
  template<typename Validator, typename Var_arg>
  void clean(Validator& op, Var_arg var_arg)
  {
    if ((mutexs_ == NULL) || (buckets_.size() != num_buckets_))
      return;

    for (size_t i = 0; i < num_buckets_; ++i)
    {
      // Use write lock for exclusive write.
      write_lock_type lock(mutexs_[i % num_mutexs_]);

      bucket_type& bucket = buckets_[i];
      size_t bucket_size = bucket.size();
      for (size_t j = 0; j < bucket_size; ++j)
      {
        if (op.valid_check(bucket[j].first, bucket[j].second, var_arg))
        {
          if (j != bucket_size - 1)
            bucket[j] = bucket[bucket_size - 1];

          bucket.pop_back();
          --bucket_size;
        }
      }
    }
  }

private:
  /// Get hash value for general type.
  template <typename T>
  size_t calculate_hash_value(const T& t)
  {
    return boost::hash_value(t);
  }

  /// Get hash value for byte_string.
  size_t calculate_hash_value(byte_string& v)
  {
    return v.hash_value();
  }

  /// Calculate the hash size for the specified number of elements.
  static size_t hash_size(size_t num_elements)
  {
    size_t nth_size = sizeof(HASH_TABLE_SIZES) / sizeof(size_t) - 1;

    for (size_t i = 0; i < nth_size; ++i)
      if (num_elements <= HASH_TABLE_SIZES[i])
      {
        nth_size = i;
        break;
      }

    return HASH_TABLE_SIZES[nth_size];
  }

  /// Create buckets and mutexs.
  void init()
  {
    if (mutexs_ != NULL)
      return;

    mutexs_ = new mutex_type[num_mutexs_];
    buckets_.resize(num_buckets_);
  }
  
  /// Remove all entries from the map.
  void clear()
  {
    // Clear all buckets to empty.
    reset();

    buckets_.clear();

    if (mutexs_ != NULL)
    {
      delete[] mutexs_;
      mutexs_ = NULL;
    }
  }

private:
  /// The mutexs in the hash.
  mutex_type* mutexs_;

  /// The buckets in the hash.
  std::vector<bucket_type> buckets_;

  // The number of mutexs in the hash.
  size_t num_mutexs_;

  // The number of buckets in the hash.
  size_t num_buckets_;
};

} // namespace bastool

#endif // BASTOOL_HASH_MAP_HPP
