#pragma once
#include <semaphore.h>
#include <cstdlib>    // abort
#include <limits.h> // SEM_VALUE_MAX

namespace ipc {
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


} // end ns
