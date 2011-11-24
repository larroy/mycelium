/*
 * Copyright 2007 Pedro Larroy Tovar
 *
 * This file is subject to the terms and conditions
 * defined in file 'LICENSE.txt', which is part of this source
 * code package.
 */

/**
 * @addtogroup utils
 * @brief misc util functions
 * @{
 */

#ifndef utils_hh
#define utils_hh
#include <cassert>
#include <stdexcept>
#include <stdint.h>
#include <semaphore.h>
#include <cerrno>
#include <sstream>
#include <vector>
#include <iostream>
#include <cstring>  // strerror
#include <cstdlib>    // abort
#include <memory>    // auto_ptr
#include <limits>

#include <limits.h> // SEM_VALUE_MAX

//#include <cctype>


#ifdef VERBOSE_DEBUG
#define DLOG(x) x
#define D(x) x
#else
#define DLOG(x)
#define D(x)
#endif

#define MIN(a,b) ((a) <= (b) ? (a) : (b))

#if __GNUC__ > 2 || __GNUC_MINOR__ >= 96
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#else
#error checkme
#endif


//#include "Url.hh"

namespace utils {
    /// execute program with execvp in a child process and wait for termination
    int system_exec(const char* cmd, ...);
    //bool inflate(const char* file, const char* dst);

    /// execute program redirecting stdout to os
    int stdout2stream_system_exec(std::ostream& os, const char* cmd, ...);
    /// handler for system call errors, currently throws std::runtime_error
    int err_sys(std::string s);

    /// recursively create the specified path
    void create_directories(const char *path);

    int get_sha1_from_hex(const char *hex, unsigned char *sha1);
    char * sha1_to_hex(const unsigned char *sha1);
    unsigned hexval(char c);

    typedef enum scheme_t {
        SCHEME_HTTP=0,
        SCHEME_FILE=1,
        SCHEME_FTP=2,
        SCHEME_UNSUPPORTED=255,
    } scheme_t;

    /// @return auto_ptr to ostringstream
    std::auto_ptr<std::ostringstream> getoss();

    /// @return hexdump of str
    std::string makeHexDump(const std::string& str);

    int scheme_string_2_int(const std::string& scheme);
    std::string scheme_int_2_string(int);


    /// get timestamp from localtime
    class Timestamp {
    friend std::ostream& operator<<(std::ostream& os, const Timestamp& t);
    public:
        static std::string get();

    };

    template<class To, typename From> To safe_cast(From from) {
        assert(sizeof(From) <= sizeof(To));
        return static_cast<To>(from);
    }
/**
 * IPC
 */

/**
 * @brief Exception thrown to denote the operation can be retried.
 * Currently used when trying to wait on a busy semaphore
 */
class AgainEx : public std::runtime_error {
    public:
        AgainEx() : std::runtime_error("unspecified") {}
        AgainEx(const std::string& arg) : std::runtime_error(arg) {}
};

class TimeOutEx : public std::runtime_error {
    public:
        TimeOutEx() : std::runtime_error("unspecified") {}
        TimeOutEx(const std::string& arg) : std::runtime_error(arg) {}
};


/**
 * @brief Interface for POSIX semaphore
 */
class Semaphore {
    public:
        Semaphore(int value, int pshared=0) throw(std::runtime_error) : sem() {
            if(value > SEM_VALUE_MAX)
                throw std::runtime_error("Number of elements too large");
            DLOG(std::cout << "Semaphore(" << value << "," << pshared << ")" << std::endl;);
            if(sem_init(&sem, pshared, value) == -1)  {
                std::ostringstream os;
                os << "sem_init error: " << std::strerror(errno);
                throw std::runtime_error("sem_init error");
            }
        }
        ~Semaphore() throw(std::runtime_error) {
            if(sem_destroy(&sem) == -1) {
                std::ostringstream os;
                os << "sem_destroy error: " << std::strerror(errno);
                std::cerr << os.str() << std::endl;
                abort();
//                throw std::runtime_error(os.str());
            }
        }

        void wait() throw(std::runtime_error) {
//            DLOG(std::cout << "wait" << std::endl;)
            if (sem_wait(&sem) == -1)
                throw std::runtime_error("sem_wait error");
        }

        int trywait() throw(AgainEx,std::runtime_error) {
            int        rc;
            if ( (rc = sem_trywait(&sem)) == -1) {
                if(errno == EAGAIN)
                    throw AgainEx("sem_trywait can't wait without blocking");
                else
                    throw std::runtime_error(("sem_trywait error"));
            }
            return(rc);
        }

        void post() throw(std::runtime_error) {
//            DLOG(std::cout << "post" << std::endl;)
            if (sem_post(&sem) == -1)
                throw std::runtime_error("sem_post error");
        }

        void timedwait(const struct timespec *abs_timeout) throw(TimeOutEx,std::runtime_error) {
            if(sem_timedwait(&sem,abs_timeout) == -1) {
                if(errno == ETIMEDOUT)
                    throw TimeOutEx("sem_timedwait timeout");
                else
                    throw std::runtime_error("sem_timedwait error");
            }
        }
        int getvalue() throw(std::runtime_error) {
            int val;
            if( sem_getvalue(&sem,&val) == -1)
                throw std::runtime_error("sem_getvalue error");
            return(val);
        }

    private:
        sem_t sem;
};


/**
 * @class CircQueue
 * @brief Circular Queue implemented with posix semaphores
 */
template<class T> class CircQueue {
    public:
        CircQueue(size_t nelems) throw(std::runtime_error) :
             mutex(1), nempty(nelems), nstored(0), buff(nelems), readpos(0), writepos(0), nelems(nelems) {
            // buff = new T[nelems];
        }
        ~CircQueue() {};

        void push(T el) throw(std::runtime_error) {
            nempty.wait();
            mutex.wait();

            buff[writepos] = el;
            writepos = (writepos + 1) % nelems;

            mutex.post();
            nstored.post();
        }

        void trypush(T el) throw(AgainEx,std::runtime_error) {
            nempty.trywait();
            mutex.wait();

            buff[writepos] = el;
            writepos = (writepos + 1) % nelems;

            mutex.post();
            nstored.post();
        }

        void timedpush(T el, const struct timespec *abs_timeout) throw(TimeOutEx,std::runtime_error) {
            nempty.timedwait(abs_timeout);
            mutex.wait();

            buff[writepos] = el;
            writepos = (writepos + 1) % nelems;

            mutex.post();
            nstored.post();
        }

        T pop() throw(std::runtime_error) {
            T el;
            nstored.wait();
            mutex.wait();

            el = buff[readpos];
            readpos = (readpos + 1) % nelems;

            mutex.post();
            nempty.post();
            return el;
        }

        T trypop() throw(AgainEx,std::runtime_error) {
            T el;
            nstored.trywait();
            mutex.wait();

            el = buff[readpos];
            readpos = (readpos + 1) % nelems;

            mutex.post();
            nempty.post();
            return el;
        }

        T timedpop(const struct timespec *abs_timeout) throw(TimeOutEx,std::runtime_error) {
            T el;
            nstored.timedwait(abs_timeout);
            mutex.wait();

            el = buff[readpos];
            readpos = (readpos + 1) % nelems;

            mutex.post();
            nempty.post();
            return el;
        }

        /**
         * size of the queue
         */
        int size() const {
            //mutex.wait();
            size_t sz = buff.size();
            //mutex.post();
            return sz;
        }

        /**
         * Number of free slots
         */
        int free_slots() { return nempty.getvalue(); }

        /**
         * Elements in the queue
         */
        int stored() { return nstored.getvalue(); }

        /**
         * True if the queue has no elements, false otherwise
         */
        bool empty() { return (nstored.getvalue() == 0); }


    private:
        Semaphore mutex;
        Semaphore nempty;
        Semaphore nstored;
        std::vector<T> buff;
        //T* buff;
        size_t    readpos;
        size_t    writepos;
        size_t    nelems;
};

/**
 * from network order (msb first) Big endian
 * @param b pointer to buffer to read from.
 */
template<typename T>T int_read(const void* b) {
    const unsigned char * p = (const unsigned char*) b; // prevent signed arithmetic
    T res = 0;
    size_t bits = sizeof(T) * 8;
    for(size_t i = 0; i < sizeof(T); ++i) {
        bits -= 8;
        res |= ((T)p[i] << bits);
    }
    return res;
}

/**
 * to network order (msb first) Big endian
 * @param b  pointer to buffer to write value. from b[0] to b[sizeof(T)-1] will get written
 * it's not safe to use this function for unsigned types
 */
template<typename T>void int_put(void* b, T t) {
    unsigned char * p = (unsigned char*) b; // prevent signed arithmetic
    uint64_t tmp;
    if(t < 0) {
        t = 0 - t;
        tmp = 0 - (uint64_t)t;
    }
    size_t bits = sizeof(T) * 8;
    bzero(b, sizeof(T));
    for(size_t i = 0; i < sizeof(T); ++i) {
        bits -= 8;
        p[i] = (tmp >> bits) & 0xff;
    }
}

    /// Convert 2 hexadecimal digits to number
    inline char x2digits_to_num(char, char) throw(std::logic_error);

    /// Convert an hexadecimal digit to number between 0 and 15
    inline char xdigit_to_num(char) throw(std::logic_error);

    /// Convert a number from 0 to 15 to hex in uppercase
    inline char digit_to_xnum(char) throw(std::logic_error);

    /// Set flag in flags
    inline void set_flag(int32_t& flags, int32_t flag);
    inline void clear_flag(int32_t& flags, int32_t flag);
    inline bool isset_flag(const int32_t& flags, int32_t flag);


    bool looks_ascii(const uint8_t *buf, size_t nbytes);
    bool looks_latin1(const uint8_t *buf, size_t nbytes);
    bool looks_extended(const uint8_t *buf, size_t nbytes);
    bool looks_utf8(const uint8_t *buf, size_t nbytes);
    enum unicode_t {
        UNKNOWN,
        UTF8,
        UTF16LE,
        UTF16BE,
        UTF32LE,
        UTF32BE
    };

    /**
     * Check for BOM
     * @return string with unicode type or NULL
     */
    const char* unicode_BOM(const uint8_t *buf, size_t nbytes);

    /**
     * Get version of PDF stored in buf
     * @return true if found
     */
    bool pdf_ver(const uint8_t *buf, size_t nbytes,int*maj,int*min);

    extern size_t allocated;

    template <class T>
    class accounting_allocator {
      public:
        // type definitions
        typedef T        value_type;
        typedef T*       pointer;
        typedef const T* const_pointer;
        typedef T&       reference;
        typedef const T& const_reference;
        typedef std::size_t    size_type;
        typedef std::ptrdiff_t difference_type;
        //static size_t allocated;

        // rebind allocator to type U
        template <class U>
        struct rebind {
            typedef accounting_allocator<U> other;
        };

        // return address of values
        pointer address (reference value) const {
            return &value;
        }
        const_pointer address (const_reference value) const {
            return &value;
        }

        /* constructors and destructor
         * - nothing to do because the allocator has no state
         */
        accounting_allocator() throw() {
        }
        accounting_allocator(const accounting_allocator&) throw() {
        }
        template <class U>
          accounting_allocator (const accounting_allocator<U>&) throw() {
        }
        ~accounting_allocator() throw() {
        }

        // return maximum number of elements that can be allocated
        size_type max_size () const throw() {
        //    std::cout << "max_size()" << std::endl;
            return std::numeric_limits<std::size_t>::max() / sizeof(T);
        }

        // allocate but don't initialize num elements of type T
        pointer allocate (size_type num, const void* = 0) {
            // print message and allocate memory with global new
            //std::cerr << "allocate " << num << " element(s)" << " of size " << sizeof(T) << std::endl;
            pointer ret = (pointer)(::operator new(num*sizeof(T)));
            //std::cerr << " allocated at: " << (void*)ret << std::endl;
           allocated += num * sizeof(T);
            //std::cerr << "allocated: " << allocated/(1024*1024) << " MB" << endl;
            return ret;
        }

        // initialize elements of allocated storage p with value value
        void construct (pointer p, const T& value) {
            // initialize memory with placement new
            new((void*)p)T(value);
        }

        // destroy elements of initialized storage p
        void destroy (pointer p) {
            // destroy objects by calling their destructor
            p->~T();
        }

        // deallocate storage p of deleted elements
        void deallocate (pointer p, size_type num) {
            // print message and deallocate memory with global delete
#if     0
            std::cerr << "deallocate " << num << " element(s)"
                      << " of size " << sizeof(T)
                      << " at: " << (void*)p << std::endl;
#endif
            ::operator delete((void*)p);
           allocated -= num * sizeof(T);
        }
    };

    template<>
     class accounting_allocator<void>
     {
     public:
       typedef size_t      size_type;
       typedef std::ptrdiff_t   difference_type;
       typedef void*       pointer;
       typedef const void* const_pointer;
       typedef void        value_type;

       template<typename _Tp1>
         struct rebind
         { typedef std::allocator<_Tp1> other; };
     };


    // return that all specializations of this allocator are interchangeable
    template <class T1, class T2>
    bool operator== (const accounting_allocator<T1>&,
                     const accounting_allocator<T2>&) throw() {
        return true;
    }
    template <class T1, class T2>
    bool operator!= (const accounting_allocator<T1>&,
                     const accounting_allocator<T2>&) throw() {
        return false;
    }

    inline void set_flag(int32_t& flags, int32_t flag) {
        flags |= flag;
    }

    inline void clear_flag(int32_t& flags, int32_t flag) {
        flags &= ~flag;
    }
    inline bool isset_flag(const int32_t& flags, int32_t flag) {
        return((flags & flag) == flag);
    }



    inline char xdigit_to_num(char c) throw(std::logic_error) {
        if( ! isxdigit(c) )
            throw std::logic_error("not an hex digit");
        return(c < 'A' ? c - '0' : toupper(c) - 'A' + 10);
    }

    inline char x2digits_to_num(char c1, char c2) throw(std::logic_error) {
        if( ! isxdigit(c1) || ! isxdigit(c2) )
            throw std::logic_error("not an hex digit");
        return((xdigit_to_num(c1) << 4) + xdigit_to_num(c2));
    }

    inline char digit_to_xnum(char c) throw(std::logic_error) {
        if( c >= 0 && c <= 15 )
            return ("0123456789ABCDEF"[(int)c]);
        else
            throw std::logic_error("digit outside [0,15] range");
    }


} // end namespace

/// Formatted string, allows to use stream operators and returns a std::string with the resulting format
#define fs(a) \
   (static_cast<const std::ostringstream&>(((*utils::getoss().get()) << a)).str ())

#endif
/** @} */

