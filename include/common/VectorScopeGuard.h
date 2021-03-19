#ifndef COMMON_VECTORSCOPEGUARD_H_
#define COMMON_VECTORSCOPEGUARD_H_

#include <stddef.h> //NULL
#include <vector>

namespace CommonNamespace {

    template<typename T> class VectorScopeGuard {
    public:

        VectorScopeGuard(std::vector<T*>* object) {
            this->object = object;
        }

        virtual std::vector<T*>* detach() {
            std::vector<T*>* ret = object;
            object = NULL;
            return ret;
        }

        virtual std::vector<T*>* getObject() {
            return object;
        }

        virtual ~VectorScopeGuard() {
            if (object != NULL) {
                for (typename std::vector<T*>::iterator i = object->begin(); i != object->end(); i++) {
                    delete *i;
                }
                delete object;
            }
        }
    private:
        VectorScopeGuard(const VectorScopeGuard&);
        VectorScopeGuard& operator=(const VectorScopeGuard&);

        std::vector<T*>* object;
    };

} /* namespace CommonNamespace */
#endif /* COMMON_VECTORSCOPEGUARD_H_ */
