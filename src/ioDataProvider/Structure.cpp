#include <ioDataProvider/Structure.h>
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

namespace IODataProviderNamespace {

    class StructurePrivate {
        friend class Structure;
    private:
        const NodeId* dataTypeId;
        const std::map<std::string, const Variant*>* fieldData;
        bool hasAttachedValues;
    };

    Structure::Structure(const NodeId& dataTypeId,
            const std::map<std::string, const Variant*>& fieldData, bool attachValues) {
        d = new StructurePrivate();
        d->dataTypeId = &dataTypeId;
        d->fieldData = &fieldData;
        d->hasAttachedValues = attachValues;
    }

    Structure::Structure(const Structure& orig) {
        // avoid self-assignment
        if (this == &orig) {
            return;
        }
        d = new StructurePrivate();
        d->dataTypeId = new NodeId(*orig.d->dataTypeId);
        std::map<std::string, const Variant*>* fieldData = new std::map<std::string, const Variant*>();
        for (std::map<std::string, const Variant*>::const_iterator i =
                orig.d->fieldData->begin(); i != orig.d->fieldData->end(); i++) {
            const std::string& fieldName = (*i).first;
            const Variant* fieldValue = (*i).second->copy();
            fieldData->insert(std::pair<std::string, const Variant*>(fieldName, fieldValue));
        }
        d->fieldData = fieldData;
        d->hasAttachedValues = true;
    }

    Structure::~Structure() {
        if (d->hasAttachedValues) {
            delete d->dataTypeId;
            for (std::map<std::string, const Variant*>::const_iterator i =
                    d->fieldData->begin(); i != d->fieldData->end(); i++) {
                delete (*i).second;
            }
            delete d->fieldData;
        }
        delete d;
    }

    const NodeId& Structure::getDataTypeId() const {
        return *d->dataTypeId;
    }

    const std::map<std::string, const Variant*>& Structure::getFieldData() const {
        return *d->fieldData;
    }

    Variant* Structure::copy() const {
        return new Structure(*this);
    }

    Variant::Type Structure::getVariantType() const {
        return STRUCTURE;
    }

    std::string Structure::toString() const {
        std::string ret("IODataProviderNamespace::Structure[dataTypeId=");
        ret.append(d->dataTypeId->toString()).append(",fieldData=");
        int index = 0;
        for (std::map<std::string, const Variant*>::const_iterator i = d->fieldData->begin();
                i != d->fieldData->end(); i++, index++) {
            if (index > 0) {
                ret.append(" ");
            }
            ret.append((*i).first).append("=>").append((*i).second->toString());
        }
        return ret.append("]");
    }

} // namespace IODataProviderNamespace
