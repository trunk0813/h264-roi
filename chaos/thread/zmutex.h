/*****************************************
**               LibChaos               **
**               zmutex.h               **
**       (c) 2013 Zennix Studios        **
*****************************************/
#ifndef ZMUTEX_H
#define ZMUTEX_H

#include "ztypes.h"

//#if COMPILER == MSVC
#if PLATFORM == WINDOWS
    #define ZMUTEX_WINTHREADS
#endif

#ifndef ZMUTEX_WINTHREADS
    #include <pthread.h>
#endif

#ifdef ZMUTEX_WINTHREADS
    struct _RTL_CRITICAL_SECTION;
    typedef _RTL_CRITICAL_SECTION CRITICAL_SECTION;
#endif

namespace LibChaos {

typedef unsigned long ztid;

// ZMutex Class
// WARNING: Relatively untested
// Allows storing of an object in a semi-thread-safe manner.
// Thread-unique locking, so only the thread that locked the mutex is allowed to unlock it.
// While the mutex is locked, other threads may get a refrence to the contained object. THREAD RESPONSIBLY. SERIOUSLY.


// An object used to control access to a resource shared by multiple threads
// Uses a pthread_mutex_t on POSIX
// Uses a Critical Section on Windows
class ZMutex {
public:
    ZMutex();
    ZMutex(const ZMutex &other);
    ~ZMutex();

    // If mutex is unlocked, mutex is locked by calling thread. If mutex is locked by other thread, function blocks until mutex is unlocked by other thread, then mutex is locked by calling thread.
    void lock();

    // Locks mutex and returns true if unlocked, else returns false.
    bool trylock();

    // Tries to lock the mutex for <timeout_microsec> microseconds, then returns false.
    bool timelock(zu64 timeout_microsec);

    // If mutex is unlocked, returns true. If mutex is locked by calling thread, mutex is unlocked. If mutex is locked by other thread, blocks until mutex is unlocked by other thread.
    void unlock();

#ifndef ZMUTEX_WINTHREADS
    // Return true if this thread owns the mutex, else returns false
    bool iOwn();
    // Returns true if mutex is locked, else returns false.
    inline bool locked(){
        return (locker() != 0);
    }
    // Returns locking thread's id, or 0 if unlocked.
    inline ztid locker(){
        return owner_tid;
    }
#endif

#ifdef ZMUTEX_WINTHREADS
    CRITICAL_SECTION *handle();
#else
    inline pthread_mutex_t *handle(){
        return &_mutex;
    }
#endif

private:
#ifdef ZMUTEX_WINTHREADS
    struct MutexData;
    MutexData *_mutex;
#else
    pthread_mutex_t _mutex;
    ztid owner_tid;
#endif
//    zu32 lock_depth;
};

// //////////////////////////////////////////////////////////////////////////////

template <class T> class ZMutexV : public ZMutex {
public:
    ZMutexV() : ZMutex(), obj(){}
    ZMutexV(const T &o) : ZMutex(), obj(o){}

    // Block until mutex owned, return refrence to obj
    T &lockdata(){
        lock();
        return obj;
    }

    // Return refrence to obj. Thread responsibly.
    inline T &data(){
        return obj;
    }

private:
    T obj;
};

// //////////////////////////////////////////////////////////////////////////////

class ZCriticalSection {
public:
    ZCriticalSection(ZMutex *mutex) : _mutex(mutex){
        if(_mutex)
            _mutex->lock();
    }
    ZCriticalSection(ZMutex &mutex) : ZCriticalSection(&mutex){

    }
    ~ZCriticalSection(){
        if(_mutex)
            _mutex->unlock();
    }

private:
    ZMutex *_mutex;
};

} // namespace LibChaos

#endif // ZMUTEX_H
