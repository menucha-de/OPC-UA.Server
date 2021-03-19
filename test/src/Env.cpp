#include "Env.h"
#include "../src/utilities/linux.h" // getAppPath

namespace TestNamespace {

    class EnvPrivate {
        friend class Env;
    private:
        std::string* appPath;
    };

    const char* Env::HOST = "127.0.0.1"; // same as in resources/conf/provider/binary/config.xml
    const int Env::PORT1 = 9090; // same as in resources/conf/provider/binary/config.xml

    Env::Env() {
        d = new EnvPrivate();
        char* appPath = getAppPath();
        d->appPath = new std::string(appPath);
        delete[] appPath;
    }

    Env::~Env() {
        delete d->appPath;
        delete d;
    }

    std::string& Env::getApplicationDir() {
        return *d->appPath;
    }
}
