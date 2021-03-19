#include "CppUTest/TestHarness.h"
#include <common/logging/ConsoleLoggerFactory.h>
#include <common/logging/Logger.h>
#include <common/logging/LoggerFactory.h>

using namespace CommonNamespace;

namespace TestNamespace {

    TEST_GROUP(CommonLogging_ConsoleLoggerFactory) {

        class LoggerImpl : public Logger {
        public:

            LoggerImpl(const char* name, bool isErrorEnabled) {
                this->errorEnabled = isErrorEnabled;
            }

            virtual void error(const char* format, ...) {
            }

            virtual void warn(const char* format, ...) {
            }

            virtual void info(const char* format, ...) {
            }

            virtual void debug(const char* format, ...) {
            }

            virtual void trace(const char* format, ...) {
            }

            virtual bool isErrorEnabled() {
                return errorEnabled;
            }

            virtual bool isWarnEnabled() {
                return true;
            }

            virtual bool isInfoEnabled() {
                return true;
            }

            virtual bool isDebugEnabled() {
                return true;
            }

            virtual bool isTraceEnabled() {
                return true;
            }
        private:
            bool errorEnabled;
        };

        class MyConsoleLoggerFactory : public ConsoleLoggerFactory {
        public:

            MyConsoleLoggerFactory(bool isErrorEnabled) : ConsoleLoggerFactory() {
                errorEnabled = isErrorEnabled;
            }

            // interface ConsoleLoggerFactory

            virtual CommonNamespace::Logger* createLogger(const char* name) {
                return new LoggerImpl(name, errorEnabled);
            }
        private:
            bool errorEnabled;
        };
    };

    TEST(CommonLogging_ConsoleLoggerFactory, GetLogger) {
        CHECK_TRUE(NULL == LoggerFactory::getLogger("a"));
        // create a logger factory
        MyConsoleLoggerFactory clf1(false /*isErrorEnabled*/);
        LoggerFactory lf1(clf1);
        // create a logger
        Logger* log1 = LoggerFactory::getLogger("a");
        CHECK_FALSE(log1->isErrorEnabled());
        // get the same logger again
        Logger* log2 = LoggerFactory::getLogger("a");
        CHECK_TRUE(log1 == log2);
        // get a new logger
        log2 = LoggerFactory::getLogger("b");
        CHECK_TRUE(log1 != log2);
        CHECK_FALSE(log2->isErrorEnabled());
    }
}
