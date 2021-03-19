#include "CppUTest/TestHarness.h"
#include <common/logging/ConsoleLogger.h>
#include <string>
#include <vector>

using namespace CommonNamespace;

namespace TestNamespace {

    TEST_GROUP(CommonLogging_ConsoleLogger) {

        class MyConsoleLogger : public ConsoleLogger {
        public:
            std::string timeStamp;
            std::string level;
            std::string loggerName;
            std::vector<std::string> msgs;

            MyConsoleLogger(const char* name) : ConsoleLogger(name) {
            }

            virtual void println(const char* timeStamp, const char* level, const char* loggerName,
                    const char* msg) {
                this->timeStamp = std::string(timeStamp);
                this->level = std::string(level);
                this->loggerName = std::string(loggerName);
                this->msgs.push_back(std::string(msg));
            }
        };
    };

    TEST(CommonLogging_ConsoleLogger, GetLogger) {
        // ERROR
        {
            MyConsoleLogger cl("a");
            cl.error("b");
            STRCMP_EQUAL("ERROR", cl.level.c_str());
            STRCMP_EQUAL("a", cl.loggerName.c_str());
            STRCMP_EQUAL("b", cl.msgs[0].c_str());
        }
        {
            MyConsoleLogger cl("a");
            cl.error("%s", "b");
            STRCMP_EQUAL("b", cl.msgs[0].c_str());
        }
        {
            MyConsoleLogger cl("a");
            cl.error("%s\n%s", "b", "c");
            STRCMP_EQUAL("b", cl.msgs[0].c_str());
            STRCMP_EQUAL("c", cl.msgs[1].c_str());
        }
        // WARN
        {
            MyConsoleLogger cl("a");
            cl.warn("b");
            STRCMP_EQUAL("WARN", cl.level.c_str());
            STRCMP_EQUAL("a", cl.loggerName.c_str());
            STRCMP_EQUAL("b", cl.msgs[0].c_str());
        }
        {
            MyConsoleLogger cl("a");
            cl.warn("%s", "b");
            STRCMP_EQUAL("b", cl.msgs[0].c_str());
        }
        {
            MyConsoleLogger cl("a");
            cl.warn("%s\n%s", "b", "c");
            STRCMP_EQUAL("b", cl.msgs[0].c_str());
            STRCMP_EQUAL("c", cl.msgs[1].c_str());
        }
        // INFO
        {
            MyConsoleLogger cl("a");
            cl.info("b");
            STRCMP_EQUAL("INFO", cl.level.c_str());
            STRCMP_EQUAL("a", cl.loggerName.c_str());
            STRCMP_EQUAL("b", cl.msgs[0].c_str());
        }
        {
            MyConsoleLogger cl("a");
            cl.info("%s", "b");
            STRCMP_EQUAL("b", cl.msgs[0].c_str());
        }
        {
            MyConsoleLogger cl("a");
            cl.info("%s\n%s", "b", "c");
            STRCMP_EQUAL("b", cl.msgs[0].c_str());
            STRCMP_EQUAL("c", cl.msgs[1].c_str());
        }
        // DEBUG
        {
            MyConsoleLogger cl("a");
            cl.debug("b");
            STRCMP_EQUAL("DEBUG", cl.level.c_str());
            STRCMP_EQUAL("a", cl.loggerName.c_str());
            STRCMP_EQUAL("b", cl.msgs[0].c_str());
        }
        {
            MyConsoleLogger cl("a");
            cl.debug("%s", "b");
            STRCMP_EQUAL("b", cl.msgs[0].c_str());
        }
        {
            MyConsoleLogger cl("a");
            cl.debug("%s\n%s", "b", "c");
            STRCMP_EQUAL("b", cl.msgs[0].c_str());
            STRCMP_EQUAL("c", cl.msgs[1].c_str());
        }
        // TRACE
        {
            MyConsoleLogger cl("a");
            cl.trace("b");
            STRCMP_EQUAL("TRACE", cl.level.c_str());
            STRCMP_EQUAL("a", cl.loggerName.c_str());
            STRCMP_EQUAL("b", cl.msgs[0].c_str());
        }
        {
            MyConsoleLogger cl("a");
            cl.trace("%s", "b");
            STRCMP_EQUAL("b", cl.msgs[0].c_str());
        }
        {
            MyConsoleLogger cl("a");
            cl.trace("%s\n%s", "b", "c");
            STRCMP_EQUAL("b", cl.msgs[0].c_str());
            STRCMP_EQUAL("c", cl.msgs[1].c_str());
        }

        MyConsoleLogger cl("a");
        CHECK_TRUE(cl.isErrorEnabled());
        CHECK_TRUE(cl.isWarnEnabled());
        CHECK_TRUE(cl.isInfoEnabled());
        CHECK_TRUE(cl.isDebugEnabled());
        CHECK_TRUE(cl.isTraceEnabled());
    }
}
