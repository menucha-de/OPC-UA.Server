#include "ParamId.h"
#include <sstream> // std::ostringstream

#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

class ParamIdPrivate {
    friend class ParamId;
private:
    ParamId::Type paramIdType;
    int namespaceIndex;
    bool hasAttachedValues;

    union {
        unsigned long numericId;
        const std::string* stringId;
    };

	void setNamespaceIndex(int index);
    void setNumeric(long id);
    void setString(const std::string& id);
    void init(bool attachValues);
    void clear();
};

ParamId::ParamId(int namespaceIndex, unsigned long id) {
    d = new ParamIdPrivate();
    d->init(false /* attachValues */);
    d->setNamespaceIndex(namespaceIndex);
    d->setNumeric(id);
}

ParamId::ParamId(int namespaceIndex, const std::string& id, bool attachValues) {
    d = new ParamIdPrivate();
    d->init(attachValues);
    d->setNamespaceIndex(namespaceIndex);
    d->setString(id);
}

ParamId::ParamId(const ParamId& orig) {
    d = new ParamIdPrivate();
    d->paramIdType = orig.d->paramIdType;
    d->hasAttachedValues = true;
    d->namespaceIndex = orig.d->namespaceIndex;
    switch (d->paramIdType) {
        case NUMERIC:
            d->numericId = orig.d->numericId;
            break;
        case STRING:
            d->stringId = new std::string(*orig.d->stringId);
            break;
    }
}

ParamId::ParamId(std::string fullString){
	//TODO ErrorHandling improvement
	d = new ParamIdPrivate();
	d->init(false);
	std::vector<std::string> v = split(fullString, '|');

	//Must have 3 elements
	if (v.size() > 2){
		//Namespace
		std::vector<std::string> vns = split(v.at(0), 'S');
		//Must have 2 elements
		if (vns.size()>1){
			std::istringstream(vns.at(1)) >> d->namespaceIndex;
		}
		//Type
		if (v.at(1).compare("Numeric") == 0){
			d->paramIdType = NUMERIC;
		} else {
			d->paramIdType = STRING;
		}
		//Identifier
		switch (d->paramIdType) {
			case NUMERIC:
				std::istringstream(v.at(2)) >> d->numericId;
				break;
			case STRING:
				d->setString(v.at(2));
				break;
		}
	}
}

std::vector<std::string> ParamId::split(const std::string& s, char d){
	std::vector<std::string> res;
	std::string sub;
	std::istringstream subStream(s);
	while (std::getline(subStream,sub, d)){
		res.push_back(sub);
	}
	return res;
}

ParamId::~ParamId() {
    d->clear();
    delete d;
}

ParamId::Type ParamId::getParamIdType() const {
    return d->paramIdType;
}

int ParamId::getNamespaceIndex() const {
    return d->namespaceIndex;
}

unsigned long ParamId::getNumeric() const {
    return d->numericId;
}

const std::string& ParamId::getString() const {
    return *d->stringId;
}

std::string ParamId::toString() const {
    std::ostringstream msg;
    msg << "NS" << d->namespaceIndex;
    msg << "|";
    switch (d->paramIdType) {
        case NUMERIC:
            msg << "Numeric|" << d->numericId;
            break;
        case STRING:
            msg << "String|" << d->stringId->c_str();
            break;
    }
    return msg.str();
}

void ParamIdPrivate::setNamespaceIndex(int index) {
    namespaceIndex = index;
}

void ParamIdPrivate::setNumeric(long id) {
    clear();
    paramIdType = ParamId::NUMERIC;
    numericId = id;
}

void ParamIdPrivate::setString(const std::string& id) {
    clear();
    paramIdType = ParamId::STRING;
    stringId = &id;
}

void ParamIdPrivate::init(bool attachValues) {
    paramIdType = ParamId::NUMERIC;
    hasAttachedValues = attachValues;
}

void ParamIdPrivate::clear() {
    if (hasAttachedValues) {
        switch (paramIdType) {
            case ParamId::NUMERIC:
                break;
            case ParamId::STRING:
                delete stringId;
                break;
        }
    }
}
