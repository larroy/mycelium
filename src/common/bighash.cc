/*
 * Copyright 2007 Pedro Larroy Tovar
 *
 * This file is subject to the terms and conditions
 * defined in file 'LICENSE.txt', which is part of this source
 * code package.
 *
 */

#include "bighash.hh"
#include <openssl/sha.h>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <boost/filesystem.hpp>
#include <dirent.h>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <ctime>
#include <cstring>  // strerror

#define DEFAULT_MASK    00644
#define LOCK_EXT ".lock"

/// Formatted string, allows to use stream operators and returns a std::string with the resulting format
#define fs(a) \
   (static_cast<const std::ostringstream&>(((*getoss().get()) << a)).str ())



namespace bfs = boost::filesystem;
using bfs::path;

using namespace std;
using namespace big_hash;

static std::auto_ptr<std::ostringstream> getoss()
{
   return std::auto_ptr<std::ostringstream> (new std::ostringstream);
}

namespace big_hash {

static int read_key(const std::string& bucket_dir, std::string& key);

static bool running_pid(pid_t pid);
static void write_pid(int fd);
static int err_sys(std::string s);

static char * sha1_to_hex(const unsigned char *sha1)
{
    static char buffer[50];
    static const char hex[] = "0123456789abcdef";
    char *buf = buffer;
    int i;

    for (i = 0; i < 20; i++) {
        unsigned int val = *sha1++;
        *buf++ = hex[val >> 4];
        *buf++ = hex[val & 0xf];
    }
    *buf++ = '\0';
    return buffer;
}

static void create_directories(const char *path)
{
    int res=0;
    int len = strlen(path);
    std::string buf;
    const char *slash = path;
    if( access(path, X_OK) == 0 )
        return;

    while (true) {
        slash = strchr(slash+1, '/');
        if( slash != NULL )
            len = slash - path;
        else
            len = strlen(path);
        buf.assign(path,len);
        res = mkdir(buf.c_str(), 00775);
        if( res != 0 && errno != EEXIST)
            err_sys(fs("mkdir \"" << buf << "\""));
        if(!slash)
            break;
    }
}


static int err_sys(std::string s) {
    std::string err;
    char *err_str = std::strerror(errno);
    if( err_str )
        err = s + " error: " + std::strerror(errno)  + "\n";
    else
        err = s + " error: unknown (strerror returned NULL)\n";
    throw std::runtime_error(err);
}

template<class To, typename From> To safe_cast(From from) {
    assert(sizeof(From) <= sizeof(To));
    return static_cast<To>(from);
}


big_hash_bucket::big_hash_bucket(const char* root_dir, const std::string& key) :
    m_lock_file(),
    m_lockfd(-1),
    m_key(key),
    m_bucket_dir(),
    block(true),
    erased(false)
{
    unsigned char sha1[SHA_DIGEST_LENGTH];
    assert(root_dir);

    if ( key.empty() )
        clog << fs(__FUNCTION__ << " Warning: empty key") << endl;

    int res;

    SHA_CTX c;
    SHA1_Init(&c);
    SHA1_Update(&c, key.c_str(), key.size());
    SHA1_Final(sha1, &c);

    char* sha1_hex = sha1_to_hex(sha1);
    string toplevel(sha1_hex,2);
    string rest(sha1_hex,2, (SHA_DIGEST_LENGTH<<1)-2);
    int bucket = 0;
    path base_path(root_dir);
    base_path /= toplevel;
    create_directories(base_path.string().c_str());
    base_path /= rest;
    for(bucket=0 ; bucket < MAXBUCKETS; ++bucket) {
        char buck[13];
        snprintf(buck, 13, "-%d", bucket);
        path buckdir(base_path.string() + buck);
        m_lock_file = buckdir.string() + LOCK_EXT;
        lock();
        //get_lockfile(m_lock_file.c_str());
        if( access(buckdir.string().c_str(), X_OK) == 0  ) {
            // access see if its a collision
            string buck_key;
            res = read_key(buckdir.string(), buck_key);
            if( res == 0 ) {
                if( buck_key == key ) {
                    m_bucket_dir.assign(buckdir.string());
                    return;
                } else
                    // Hash collision (unlikely) or corrupt data (likely)
                    unlock();
                    continue;
            } else {
                // we couldn't read key, use bucket
                m_bucket_dir.assign(buckdir.string());
                write_key();
                return;
            }
        } else if ( errno == ENOENT ) {
            create_directories(buckdir.string().c_str());
            m_bucket_dir.assign(buckdir.string());
            write_key();
            return;
        } else
            throw std::runtime_error(fs("can't access: "<< buckdir.string()));
    }
    throw std::runtime_error("out of buckets");
}

big_hash_bucket::big_hash_bucket(const char* bucket, bool block) :
    m_lock_file(),
    m_lockfd(-1),
    m_key(),
    m_bucket_dir(bucket),
    block(block),
    erased(false)
{

    while (! m_bucket_dir.empty() && *m_bucket_dir.rbegin() == '/')
        m_bucket_dir = m_bucket_dir.substr(0,m_bucket_dir.size()-1);
    if ( m_bucket_dir.empty() )
        throw runtime_error("empty bucket");

    path buckdir(m_bucket_dir);
    m_lock_file = buckdir.string() + LOCK_EXT;
    path keyf = buckdir / KEY_FNAME;

    if ( access(buckdir.string().c_str(), X_OK) != 0  )
        err_sys(fs("access "<<buckdir.string()));

    if ( access(keyf.string().c_str(), R_OK|W_OK) != 0 )
        throw runtime_error(fs("missing key file"<<buckdir.string()));

    lock();
    //get_lockfile(m_lock_file.c_str());

    if ( read_key(buckdir.string(), m_key))
        throw runtime_error(fs("can't read key: "<<buckdir.string()));

}

void big_hash_bucket::lock()
{
    if( ! m_lock_file.empty() )
        get_lockfile(m_lock_file.c_str());
    else
        throw runtime_error("Empty m_lock_file");
}

void big_hash_bucket::unlock()
{
    if( m_lockfd > 0 ) {
        //if(close(m_lockfd))
        //    err_sys("close");
        close(m_lockfd);
        m_lockfd = -1;
    }
    if(! m_lock_file.empty()) {
        if(unlink(m_lock_file.c_str()))
            err_sys("unlink");
        m_lock_file.clear();
    }
}

void big_hash_bucket::erase()
{
    if ( ! locked() )
        lock();
    bfs::remove_all(m_bucket_dir);
    unlock();
    erased = true;
}

big_hash_bucket::~big_hash_bucket()
{
    unlock();
}

std::string big_hash_bucket::get()
{
    if ( erased )
        throw runtime_error(fs(__FUNCTION__ << ": bucket has been erased"));

    string value;
    path valuef(m_bucket_dir);
    valuef /= VALUE_FNAME;
    if ( bfs::is_regular_file(valuef) )
        value.reserve(bfs::file_size(valuef));
    else
        return value;

    int valuefd = open(valuef.string().c_str(), O_RDONLY);
    if ( valuefd < 0 ) {
        if ( errno != ENOENT )
            throw key_error(fs("Can't open value file: \"" << valuef.string() << "\": " << std::strerror(errno)));
        else
            return value;
    }


#define BSIZE 8192
    char buf[BSIZE];
    ssize_t nread=0;
    while( (nread=read(valuefd, buf, BSIZE)) > 0 ) {
        value.append(buf,nread);
    }
#undef BSIZE
    if (nread < 0) {
        close(valuefd);
        err_sys("read");
    }
    close(valuefd);

    return value;
}

int read_key(const std::string& bucket_dir, std::string& key)
{
    int result=0;
    key.clear();
    path keyf(bucket_dir);
    keyf /= KEY_FNAME;
    int keyfd = open(keyf.string().c_str(), O_RDWR);
    if( keyfd < 0 )
        return -1;
#define BSIZE 1024
    char buf[BSIZE];
    ssize_t nread=0;
    while( (nread=read(keyfd, buf, BSIZE)) > 0 ) {
        key.append(buf,nread);
    }
#undef BSIZE
    if (nread < 0)
        result = -1;
    else
        result = 0;
    close(keyfd);
    return result;
}

void big_hash_bucket::get_lockfile(const char* f)
{
    while (true) {
        if (m_lockfd > 0)
            close(m_lockfd);

        m_lockfd = open(f, O_CREAT | O_EXCL | O_RDWR , DEFAULT_MASK);

        // store pid in lockfile and look if it's stale
        if ( m_lockfd < 0 && errno == EEXIST ) {
            m_lockfd = open(f, O_RDWR);
            if ( m_lockfd < 0 )
                err_sys("open");

            char buf[64];
            memset(buf, 0, 64);
            if ( read(m_lockfd, buf, 63) <= 0 ) {
                clog << "lockfile w/o pid: " << f << " reusing..." << endl;
                write_pid(m_lockfd);
                break;
            }

            pid_t lock_pid = atol(buf);
            pid_t our_pid = getpid();

            if ( lock_pid == our_pid ) {
                clog << "Already locked by our pid: " << our_pid << endl;
                throw std::runtime_error("prevent deadlock");
                break;
            }

            if ( ! running_pid(lock_pid) ) {
                clog << "Stale lockfile: " << f << " reusing..." << endl;

                write_pid(m_lockfd);
                break;
            }


            if ( ! block )
                throw runtime_error(fs("lock on " << f << " would block and block is false"));
            clog << "big_hash_bucket::get_lockfile :" << f << " locked... sleeping" << endl;
            struct timespec ts;
            memset(&ts, 0, sizeof(timespec));
            ts.tv_sec = 1;
            //int ret = sleep(1);
            if ( nanosleep(&ts, 0) )
                err_sys("nanosleep");

            continue;

        } else if( m_lockfd < 0 && errno != EEXIST)
            err_sys(f);

        else {
            write_pid(m_lockfd);
            break;
        }
    }

    if (m_lockfd > 0) {
        close(m_lockfd);
        m_lockfd = -1;
    }

    return;
}

void big_hash_bucket::write_key()
{
    path keyf(m_bucket_dir);
    keyf /= KEY_FNAME;
    int keyfd = open(keyf.string().c_str(), O_WRONLY | O_TRUNC | O_CREAT, DEFAULT_MASK);
    if( keyfd < 0 )
        err_sys("open");
    const char* b=0;
    const char* e=0;
    ssize_t nwrite=0;
    b = m_key.c_str();
    e = m_key.c_str() + m_key.size();
#define BSIZE 8192
    while( e>b && (nwrite=write(keyfd, b, e-b>BSIZE ? BSIZE : e-b)) > 0 )
        b += nwrite;
#undef BSIZE
    if (nwrite < 0)
        //result = -1;
        err_sys("write");
    close(keyfd);
    return;
}

void big_hash_bucket::set(const std::string& value)
{
    if ( erased )
        throw runtime_error(fs(__FUNCTION__ << ": bucket has been erased"));

    if ( ! locked() )
        lock();

    path valuef(m_bucket_dir);
    valuef /= VALUE_FNAME;
    int valuefd = open(valuef.string().c_str(), O_WRONLY | O_TRUNC | O_CREAT, DEFAULT_MASK);
    if( valuefd < 0 )
        throw runtime_error(fs(__FUNCTION__ << ": open"));
    const char* b=0;
    const char* e=0;
    ssize_t nwrite=0;
    b = value.c_str();
    e = value.c_str() + value.size();
#define BSIZE 8192
    while( e>b && (nwrite=write(valuefd, b, e-b>BSIZE ? BSIZE : e-b)) > 0 )
        b += nwrite;
#undef BSIZE
    if (nwrite < 0) {
        //result = -1;
        bfs::remove(valuef);
        err_sys("write");
    }
    close(valuefd);
    return;
}


static bool running_pid(pid_t pid)
{
    bool found = false;
    static const char * const dir = "/proc";
    DIR* procdir = opendir(dir);
    if ( ! procdir )
        err_sys("opendir proc");

    const struct dirent * ent = 0;
    pid_t curpid = 0;
    while ( (ent = readdir(procdir)) ) {
        if ( ent->d_name[0] > 0 && ent->d_name[0] <= '9' ) {
            curpid = atol(ent->d_name);
            if (curpid == pid) {
                found = true;
                break;
            }
        }
    }
    closedir(procdir);
    return found;
}


static void write_pid(int fd)
{
    char buf[64];
    memset(buf, 0, 64);
    int res;
    if ( lseek(fd, 0, SEEK_SET) != 0 )
        err_sys("lseek");

    if ( (res = snprintf(buf, 63, "%ld", safe_cast<unsigned long>(getpid()))) < 0 )
        err_sys("snprintf");

    ssize_t cnt;
    if ( (cnt = write(fd, buf, res)) < res )
        err_sys("write");

    if ( ftruncate(fd, res) < 0 )
        err_sys("ftruncate");
}

}; // end namespace big_hash

#ifdef EXPORT_PYTHON_INTERFACE
#include <boost/python.hpp>
using namespace boost::python;

static void key_error_translator(const key_error& e)
{
    PyErr_SetString(PyExc_KeyError, e.what());
}

BOOST_PYTHON_MODULE_INIT(bighash)
{
    class_<big_hash_bucket, boost::noncopyable>("BigHash", "Hash like disk based storage, locks when created, unlocks when destructed", init<const char*, bool>())
        .def(init<const char*, const std::string&>())
        .def("key", &big_hash_bucket::key, "return key of this bucket")
        .def("get", &big_hash_bucket::get, "return data of this bucket")
        .def("set", &big_hash_bucket::set, "set data")
        .def("lock", &big_hash_bucket::lock, "lock")
        .def("unlock", &big_hash_bucket::unlock, "unlock")
        .def("bucket_dir", &big_hash_bucket::bucket_dir, "the directory for this bucket")
        .def("erase", &big_hash_bucket::erase, "erase this bucket, don't use any other function afterwards")
    ;
    register_exception_translator<key_error>(key_error_translator);
}
#endif

