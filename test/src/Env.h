#ifndef TEST_ENV_H
#define TEST_ENV_H

#include <string>

namespace TestNamespace {

    class EnvPrivate;
    
    class Env {
    public:        
        static const char* HOST;
        static const int PORT1;
        
        Env();
        ~Env();
        
        std::string& getApplicationDir();
    private:
        EnvPrivate* d;
    };
} // namespace Test
#endif /* TEST_ENV_H */

