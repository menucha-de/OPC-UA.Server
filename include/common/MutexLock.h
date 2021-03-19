#ifndef COMMON_MUTEXLOCK_H
#define COMMON_MUTEXLOCK_H

#include "Mutex.h"

namespace CommonNamespace {

    class MutexLockPrivate;

    class MutexLock {
    public:
        MutexLock(Mutex& mutex);
        virtual ~MutexLock();

        virtual void unlock();
    private:
        MutexLockPrivate* d;
    };

} // CommonNamespace
#endif /* COMMON_MUTEXLOCK_H */
