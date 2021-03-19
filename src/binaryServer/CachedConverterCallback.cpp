#include "CachedConverterCallback.h"
#include <common/Exception.h>
#include <common/Mutex.h>
#include <common/MutexLock.h>
#include <common/ScopeGuard.h>
#include <common/logging/Logger.h>
#include <common/logging/LoggerFactory.h>
#include <sasModelProvider/base/ConversionException.h>
#include <uadatetime.h>
#include <sstream> // std::ostringstream

namespace SASModelProviderNamespace {

    using namespace CommonNamespace;

    class CachedConverterCallbackPrivate {
        friend class CachedConverterCallback;
    private:
        Logger* log;
        ConverterUa2IO::ConverterCallback* callback;
        bool hasValuesAttached;

        Mutex* mutex;
        // type -> super types
        std::map<UaNodeId, std::vector<UaNodeId>*> superTypes;
        std::map<UaNodeId, UaStructureDefinition> structureDefinitions;
    };

    CachedConverterCallback::CachedConverterCallback(ConverterUa2IO::ConverterCallback& callback,
            bool attachValue) /* throws MutexException */ {
        d = new CachedConverterCallbackPrivate();
        d->log = LoggerFactory::getLogger("CachedConverterCallback");
        d->callback = &callback;
        d->hasValuesAttached = attachValue;
        d->mutex = new Mutex(); // MutexException
    }

    CachedConverterCallback::~CachedConverterCallback() {
        clear();
        if (d->hasValuesAttached) {
            delete d->callback;
        }
        delete d->mutex;
        delete d;
    }

    void CachedConverterCallback::clear() {
        MutexLock lock(*d->mutex);
        for (std::map<UaNodeId, std::vector<UaNodeId>*>::const_iterator it =
                d->superTypes.begin(); it != d->superTypes.end(); it++) {
            delete it->second;
        }
        d->superTypes.clear();
        d->structureDefinitions.clear();
    }

    void CachedConverterCallback::preload(const UaNodeId& typeId) {
        if (0 == typeId.namespaceIndex()) {
            return;
        }
        // get base type
        UaNodeId buildInDataTypeId;
        std::vector<UaNodeId>* superTypes = getSuperTypes(typeId);
        if (superTypes != NULL) {
            ScopeGuard<std::vector<UaNodeId> > superTypesSG(superTypes);
            if (superTypes->size() > 0) {
                buildInDataTypeId = superTypes->back();
            }
        }
        if (buildInDataTypeId.isNull()) {
            std::ostringstream msg;
            msg << "Cannot get base type of " << typeId.toXmlString().toUtf8();
            throw ExceptionDef(ConversionException, msg.str());
        }
        switch (buildInDataTypeId.identifierNumeric()) {
            case OpcUaId_Structure: // 22
            case OpcUaId_Union: // 12756
            {
                UaStructureDefinition sd = getStructureDefinition(typeId);
                if (!sd.isNull()) {
                    // for each field
                    for (int i = 0; i < sd.childrenCount(); i++) {
                        preload(sd.child(i).typeId());
                    }
                } else {
                    std::ostringstream msg;
                    msg << "Cannot get structure definition of " << typeId.toXmlString().toUtf8();
                    throw ExceptionDef(ConversionException, msg.str());
                }
                break;
            }
            default:
                break;
        }
    }

    UaStructureDefinition CachedConverterCallback::getStructureDefinition(const UaNodeId& typeId)
    /* throws ConversionException */ {
        MutexLock lock(*d->mutex);
        std::map<UaNodeId, UaStructureDefinition>::const_iterator it =
                d->structureDefinitions.find(typeId);
        if (it == d->structureDefinitions.end()) {
            if (d->log->isDebugEnabled()) {
                d->log->debug("Getting structure definition of %s", typeId.toXmlString().toUtf8());
            }
            UaStructureDefinition sd = d->callback->getStructureDefinition(typeId);
            if (!sd.isNull()) {
                d->structureDefinitions[typeId] = sd;
                if (d->log->isDebugEnabled()) {
                    d->log->debug("Get structure definition");
                }
                return sd;
            }
            std::ostringstream msg;
            msg << "Cannot get structure definition of " << typeId.toXmlString().toUtf8();
            throw ExceptionDef(ConversionException, msg.str());
        }
        return it->second;
    }

    std::vector<UaNodeId>* CachedConverterCallback::getSuperTypes(const UaNodeId& typeId)
    /* throws ConversionException */ {
        MutexLock lock(*d->mutex);
        std::map<UaNodeId, std::vector<UaNodeId>*>::const_iterator it = d->superTypes.find(typeId);
        if (it == d->superTypes.end()) {
            if (d->log->isDebugEnabled()) {
                d->log->debug("Getting super types of %s", typeId.toXmlString().toUtf8());
            }
            std::vector<UaNodeId>* superTypeIds = d->callback->getSuperTypes(typeId);
            if (superTypeIds != NULL) {
                d->superTypes[typeId] = superTypeIds;
                if (d->log->isDebugEnabled()) {
                    d->log->debug("Get %d super types", superTypeIds->size());
                }
                return new std::vector<UaNodeId>(*superTypeIds);
            }
            std::ostringstream msg;
            msg << "Cannot get super types of " << typeId.toXmlString().toUtf8();
            throw ExceptionDef(ConversionException, msg.str());
        }
        return new std::vector<UaNodeId>(*it->second);
    }
}