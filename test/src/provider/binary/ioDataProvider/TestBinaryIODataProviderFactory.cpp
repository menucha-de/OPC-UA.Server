#include "CppUTest/TestHarness.h"
#include "../../../../../src/provider/binary/ioDataProvider/BinaryIODataProvider.h"
#include "../../../../../src/provider/binary/ioDataProvider/BinaryIODataProviderFactory.h"
#include <common/logging/ConsoleLoggerFactory.h>
#include <common/logging/LoggerFactory.h>
#include <ioDataProvider/IODataProvider.h>

using namespace CommonNamespace;

namespace TestNamespace {

    TEST_GROUP(ProviderBinaryIoDataProvider_BinaryIODataProviderFactory) {
        ConsoleLoggerFactory clf;
        LoggerFactory* lf;

        void setup() {
            lf = new LoggerFactory(clf);
        }

        void teardown() {
            delete lf;
        }
    };

    TEST(ProviderBinaryIoDataProvider_BinaryIODataProviderFactory, Create) {
        BinaryIODataProviderFactory factory;
        std::vector<IODataProviderNamespace::IODataProvider*>* provider = factory.create();
        CHECK_TRUE(provider != NULL);
        for (std::vector<IODataProviderNamespace::IODataProvider*>::iterator i = provider->begin();
                i != provider->end(); i++) {
            delete *i;
        }
        delete provider;
    }
}
