#include <common/logging/LoggerFactory.h>
#include <common/Exception.h>
#include <common/logging/LoggingException.h>
#include <stddef.h> //NULL
#include <algorithm> //std::find
#include <vector>
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

namespace CommonNamespace {

    class LoggerFactoryPrivate {
        friend class LoggerFactory;
    private:
        static ILoggerFactory* iloggerFactory;
        static std::vector<ILoggerFactory*>* previous;

        ILoggerFactory* addedIloggerFactory;
        bool hasAttachedValues;
    };

    ILoggerFactory* LoggerFactoryPrivate::iloggerFactory = NULL;
    std::vector<ILoggerFactory*>* LoggerFactoryPrivate::previous = NULL;

    LoggerFactory::LoggerFactory(ILoggerFactory& iloggerFactory, bool attachValues) {
        d = new LoggerFactoryPrivate();
        if (d->iloggerFactory != NULL) {
            if (d->previous == NULL) {
                d->previous = new std::vector<ILoggerFactory*>();
            }
            d->previous->push_back(d->iloggerFactory);
        }
        d->iloggerFactory = &iloggerFactory;
        d->addedIloggerFactory = &iloggerFactory;
        d->hasAttachedValues = attachValues;
    }

    LoggerFactory::~LoggerFactory() {
        // if added logger is active
        if (d->iloggerFactory == d->addedIloggerFactory) {
            // if a previous logger exists
            if (d->previous != NULL) {
                // activate previous logger
                d->iloggerFactory = d->previous->back();
                // remove added logger from previous loggers
                if (d->previous->size() == 1) {
                    delete d->previous;
                    d->previous = NULL;
                } else {
                    d->previous->pop_back();
                }
            } else {
                // deactivate logging
                d->iloggerFactory = NULL;
            }
        } else {
            // remove added logger from previous loggers
            if (d->previous->size() == 1) {
                delete d->previous;
                d->previous = NULL;
            } else {
                std::vector<ILoggerFactory*>::iterator it =
                        std::find(d->previous->begin(), d->previous->end(), d->addedIloggerFactory);
                d->previous->erase(it);
            }
        }
        if (d->hasAttachedValues) {
            delete d->addedIloggerFactory;
        }
        delete d;
    }

    Logger* LoggerFactory::getLogger(const char* name) {
        if (LoggerFactoryPrivate::iloggerFactory == NULL) {
            return NULL;
        }
        return LoggerFactoryPrivate::iloggerFactory->getLogger(name);
    }
} // namespace CommonNamespace