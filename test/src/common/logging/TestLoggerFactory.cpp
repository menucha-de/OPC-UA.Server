#include "CppUTest/TestHarness.h"
#include <common/logging/Logger.h>
#include <common/logging/ILoggerFactory.h>
#include <common/logging/LoggerFactory.h>

using namespace CommonNamespace;

namespace TestNamespace {

    TEST_GROUP(CommonLogging_LoggerFactory) {

        class LoggerImpl : public Logger {
        public:

            LoggerImpl(bool isErrorEnabled) {
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

        class ILoggerFactoryImpl : public ILoggerFactory {
        public:

            ILoggerFactoryImpl(Logger& log) {
                this->log = &log;
            }

            virtual Logger* getLogger(const char* name) {
                return log;
            }
        private:
            Logger* log;
        };
    };

    TEST(CommonLogging_LoggerFactory, GetLogger) {
        CHECK_TRUE(NULL == LoggerFactory::getLogger("a"));
        // create a logger factory
        LoggerImpl l1(false /*isErrorEnabled*/);
        ILoggerFactoryImpl ilf1(l1);
        LoggerFactory* lf1 = new LoggerFactory(ilf1);
        // create a logger
        Logger* log = LoggerFactory::getLogger("a");
        CHECK_FALSE(log->isErrorEnabled());
        {
            // override first logger factory
            LoggerImpl l2(true /*isErrorEnabled*/);
            ILoggerFactory* ilf2 = new ILoggerFactoryImpl(l2);
            LoggerFactory lf2(*ilf2, true /*attachValues*/);
            log = LoggerFactory::getLogger("a");
            CHECK_TRUE(log->isErrorEnabled());
            // the second factory incl. attached ILoggerFactory is destroyed due to end of block
        }
        // the first logger factory is active again
        log = LoggerFactory::getLogger("a");
        CHECK_FALSE(log->isErrorEnabled());
        {
            // override first logger factory
            LoggerImpl l2(false /*isErrorEnabled*/);
            ILoggerFactoryImpl ilf2(l2);
            LoggerFactory* lf2 = new LoggerFactory(ilf2);
            // override second logger factory
            LoggerImpl l3(true /*isErrorEnabled*/);
            ILoggerFactoryImpl ilf3(l3);
            LoggerFactory lf3(ilf3);
            log = LoggerFactory::getLogger("a");
            CHECK_TRUE(log->isErrorEnabled());
            // delete first factory before the second/third one
            delete lf1;
            // the third factory is still active
            log = LoggerFactory::getLogger("a");
            CHECK_TRUE(log->isErrorEnabled());
            // delete second factory before the third one
            delete lf2;
            // the third factory is still active
            log = LoggerFactory::getLogger("a");
            CHECK_TRUE(log->isErrorEnabled());
            // the third factory is destroyed due to end of block
        }
        CHECK_TRUE(NULL == LoggerFactory::getLogger("a"));
    }
}
