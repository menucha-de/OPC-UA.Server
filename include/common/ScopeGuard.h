#ifndef COMMON_SCOPEGUARD_H_
#define COMMON_SCOPEGUARD_H_

#include <stddef.h> //NULL

namespace CommonNamespace {

    template<typename T> class ScopeGuard {
    public:
        ScopeGuard(T* object, bool isArray = false) {
            this->object = object;
            this->isArray = isArray;
        }
        
        virtual T* detach() {
            T* ret = object;
            object = NULL;
            return ret;
        }

        virtual T* getObject() {
            return object;
        }

        virtual ~ScopeGuard() {
            if (object != NULL) {
                if (isArray) {
                    delete[] object;
                } else {
                    delete object;
                }
            }
        }
    private:
        ScopeGuard(const ScopeGuard&);
        ScopeGuard& operator=(const ScopeGuard&);

        T* object;
        bool isArray;
    };

} /* namespace CommonNamespace */
#endif /* COMMON_SCOPEGUARD_H_ */
