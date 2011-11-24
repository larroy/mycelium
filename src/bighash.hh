/*
 * Copyright 2007 Pedro Larroy Tovar
 *
 * This file is subject to the terms and conditions
 * defined in file 'LICENSE.txt', which is part of this source
 * code package.
 */

/**
 * @addtogroup Big_hash
 * @brief Persistent hash-like data structure and associated filesystem path
 * @version 0.3
 * @author Pedro Larroy Tovar
 *
 * Hash buckets can be created based on a key. Data can be stored in the hash
 * bucket and the path of the hash bucket can be used to store additional
 * files, directories, etc.  Simple file/locking mechanism is provided to lock
 * hash entry while operating on it
 *
 * Example:
 * @code
 * big_hash_bucket b("/tmp/bh","omg");
 * @endcode
 * A big hash bucket at root directory /tmp/bh is created with key "omg"
 * Now there's a lock on this bucket and additional files can be stored in b.bucket_dir() path. Also a string of data can be associated with this key by calling b.set("somedata"), just like an associative container.
 *
 * @{
 */

#ifndef bighash_hh
#define bighash_hh
#include <string>
#include <stdexcept>

/// name of key file on disk, on bucket directory
#define KEY_FNAME "__key__"

/// name of value file on disk, on bucket directory
#define VALUE_FNAME "__value__"

/// maximum number of buckets to create with same hash in case of collision
#define MAXBUCKETS 3

namespace big_hash {

class timeout_error : public std::runtime_error {
public:
	timeout_error() : std::runtime_error("timeout_error: unspecified") {}
	timeout_error(const std::string& arg) : std::runtime_error(arg) {}
};

class key_error : public std::runtime_error {
public:
	key_error() : std::runtime_error("key_error: unspecified") {}
	key_error(const std::string& arg) : std::runtime_error(arg) {}
};


/**
 * @brief a bucket for a hashed item
 */
class big_hash_bucket
{
public:
    /**
     * @brief Create bucket in root_dir with the provided key
     * @param root_dir root directory of big_hash
     * @param key key for this bucket
     */
	big_hash_bucket(const char* root_dir, const std::string& key);

    /// Create a bucket object from the very bucket directory and lock it
	big_hash_bucket(const char* bucket, bool block);

	~big_hash_bucket();

    /// @return key of this bucket
	std::string key() const;

    /// write value ("data") of this bucket to disk
	void set(const std::string& value);

    /// @return data of this bucket
	std::string get();

	operator std::string() { return get(); }

	void operator=(const std::string& value) { set(value); }

	/// Get terminated path
	std::string bucket_dir() const { return m_bucket_dir; }

    /// lock bucket
	void lock();

    /// unlock bucket
	void unlock();

    /// @return true if locked
    bool locked() const { return ! m_lock_file.empty(); }

    /// delete bucket
    void erase();


private:
    big_hash_bucket(const big_hash_bucket&);
    const big_hash_bucket& operator=(const big_hash_bucket&);
	void write_key();
	void get_lockfile(const char* f);

	std::string m_lock_file;
	int	m_lockfd;
	std::string m_key;
	std::string m_value;
	std::string m_bucket_dir;
	bool block;
    bool erased;
};


inline std::string big_hash_bucket::key() const
{
    if ( erased )
		throw std::runtime_error("big_hash_bucket::key: bucket has been erased");
    return m_key;
}

}; // end namespace
#endif
/** @} */

