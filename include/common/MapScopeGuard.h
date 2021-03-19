#ifndef COMMON_MAPSCOPEGUARD_H_
#define COMMON_MAPSCOPEGUARD_H_

#include <stddef.h> //NULL
#include <map>

namespace CommonNamespace {

    template<typename TKey, typename TValue> class MapScopeGuard {
    public:

        MapScopeGuard(std::map<TKey, TValue*>* object) {
            this->object = object;
        }

        virtual std::map<TKey, TValue*>* detach() {
            std::map<TKey, TValue*>* ret = object;
            object = NULL;
            return ret;
        }

        virtual std::map<TKey, TValue*>* getObject() {
            return object;
        }

        virtual ~MapScopeGuard() {
            if (object != NULL) {
                for (typename std::map<TKey, TValue*>::iterator i = object->begin();
                        i != object->end(); i++) {
                    delete (*i).second;
                }
                delete object;
            }
        }
    private:
        MapScopeGuard(const MapScopeGuard&);
        MapScopeGuard& operator=(const MapScopeGuard&);

        std::map<TKey, TValue*>* object;
    };

} /* namespace CommonNamespace */
#endif /* COMMON_MAPSCOPEGUARD_H_ */
