#include <common/MutexLock.h>
#include <stddef.h> // NULL

namespace CommonNamespace {

    class MutexLockPrivate {
        friend class MutexLock;
    private:
        Mutex* mutex;
    };

    MutexLock::MutexLock(Mutex& mutex) {
        d = new MutexLockPrivate();
        d->mutex = &mutex;
        //pthread_mutex_lock(&d->mutex->getMutex());
    }

    MutexLock::~MutexLock() {
        unlock();
        delete d;
    }

    void MutexLock::unlock() {
        if (d->mutex != NULL) {
            //pthread_mutex_unlock(&d->mutex->getMutex());
            d->mutex = NULL;
        }
    }

} // CommonNamespace
