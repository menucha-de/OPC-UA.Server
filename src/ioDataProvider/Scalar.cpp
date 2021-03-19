#include <ioDataProvider/Scalar.h>
#include <sstream>
#include <stdio.h> // sprintf
#include <string.h> // std::ostringstream
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

namespace IODataProviderNamespace {

    class ScalarPrivate {
        friend class Scalar;
    private:
        Scalar::Type scalarType;
        bool hasAttachedValues;
        long byteStringLengthValue;
        const std::string* localizedTextLocaleValue;

        union {
            bool boolValue;
            signed char scharValue;
            char charValue;
            int intValue;
            unsigned int uintValue;
            long longValue;
            unsigned long ulongValue;
            long long llongValue;
            unsigned long long ullongValue;
            double floatValue;
            double doubleValue;
            const std::string* stringValue; // localizedTextText
            const char* byteStringValue;
        };

        void clear();
    };

    Scalar::Scalar() {
        d = new ScalarPrivate();
        d->scalarType = Scalar::INT;
        d->hasAttachedValues = false;
        setInt(0);
    }

    Scalar::Scalar(const Scalar& value) {
        // avoid self-assignment
        if (this == &value) {
            return;
        }
        d = new ScalarPrivate();
        d->scalarType = value.d->scalarType;
        d->hasAttachedValues = true;
        switch (d->scalarType) {
            case BOOL:
                d->boolValue = value.d->boolValue;
                break;
            case SCHAR:
                d->scharValue = value.d->scharValue;
                break;
            case CHAR:
                d->charValue = value.d->charValue;
                break;
            case INT:
                d->intValue = value.d->intValue;
                break;
            case UINT:
                d->uintValue = value.d->uintValue;
                break;
            case LONG:
                d->longValue = value.d->longValue;
                break;
            case ULONG:
                d->ulongValue = value.d->ulongValue;
                break;
            case LLONG:
                d->llongValue = value.d->llongValue;
                break;
            case ULLONG:
                d->ullongValue = value.d->ullongValue;
                break;
            case FLOAT:
                d->floatValue = value.d->floatValue;
                break;
            case DOUBLE:
                d->doubleValue = value.d->doubleValue;
                break;
            case STRING:
                d->stringValue = value.d->stringValue == NULL ? NULL
                        : new std::string(*value.d->stringValue);
                break;
            case BYTE_STRING:
            {
                d->byteStringLengthValue = value.d->byteStringLengthValue;
                char* chars = NULL;
                if (value.d->byteStringValue != NULL) {
                    chars = new char[d->byteStringLengthValue];
                    memcpy(chars, value.d->byteStringValue, d->byteStringLengthValue * sizeof (char));
                }
                d->byteStringValue = chars;
                break;
            }
            case LOCALIZED_TEXT:
                d->localizedTextLocaleValue = value.d->localizedTextLocaleValue == NULL
                        ? NULL : new std::string(*value.d->localizedTextLocaleValue);
                d->stringValue = value.d->stringValue == NULL
                        ? NULL : new std::string(*value.d->stringValue);
                break;
        }
    }

    Scalar::~Scalar() {        
        d->clear();
        delete d;
    }

    Scalar::Type Scalar::getScalarType() const {
        return d->scalarType;
    }

    bool Scalar::getBool() const {
        return d->boolValue;
    }

    void Scalar::setBool(bool value) {
        d->clear();
        d->scalarType = BOOL;
        d->boolValue = value;
    }

    signed char Scalar::getSChar() const {
        return d->scharValue;
    }

    void Scalar::setSChar(signed char value) {
        d->clear();
        d->scalarType = SCHAR;
        d->scharValue = value;
    }

    char Scalar::getChar() const {
        return d->charValue;
    }

    void Scalar::setChar(char value) {
        d->clear();
        d->scalarType = CHAR;
        d->charValue = value;
    }

    int Scalar::getInt() const {
        return d->intValue;
    }

    void Scalar::setInt(int value) {
        d->clear();
        d->scalarType = INT;
        d->intValue = value;
    }

    unsigned int Scalar::getUInt() const {
        return d->uintValue;
    }

    void Scalar::setUInt(unsigned int value) {
        d->clear();
        d->scalarType = UINT;
        d->uintValue = value;
    }

    long Scalar::getLong() const {
        return d->longValue;
    }

    void Scalar::setLong(long value) {
        d->clear();
        d->scalarType = LONG;
        d->longValue = value;
    }

    unsigned long Scalar::getULong() const {
        return d->ulongValue;
    }

    void Scalar::setULong(unsigned long value) {
        d->clear();
        d->scalarType = ULONG;
        d->ulongValue = value;
    }

    long long Scalar::getLLong() const {
        return d->llongValue;
    }

    void Scalar::setLLong(long long value) {
        d->clear();
        d->scalarType = LLONG;
        d->llongValue = value;
    }

    unsigned long long Scalar::getULLong() const {
        return d->ullongValue;
    }

    void Scalar::setULLong(unsigned long long value) {
        d->clear();
        d->scalarType = ULLONG;
        d->ullongValue = value;
    }

    float Scalar::getFloat() const {
        return d->floatValue;
    }

    void Scalar::setFloat(float value) {
        d->clear();
        d->scalarType = FLOAT;
        d->floatValue = value;
    }

    double Scalar::getDouble() const {
        return d->doubleValue;
    }

    void Scalar::setDouble(double value) {
        d->clear();
        d->scalarType = DOUBLE;
        d->doubleValue = value;
    }

    const std::string* Scalar::getString() const {
        return d->stringValue;
    }

    void Scalar::setString(const std::string* value, bool attachValue) {
        d->clear();
        d->scalarType = STRING;
        d->stringValue = value;
        d->hasAttachedValues = attachValue;
    }

    const char* Scalar::getByteString() const {
        return d->byteStringValue;
    }

    long Scalar::getByteStringLength() const {
        return d->byteStringLengthValue;
    }

    void Scalar::setByteString(const char* value, long length, bool attachValue) {
        d->clear();
        d->scalarType = BYTE_STRING;
        d->byteStringLengthValue = length;
        d->byteStringValue = value;
        d->hasAttachedValues = attachValue;
    }

    const std::string* Scalar::getLocalizedTextLocale() const {
        return d->localizedTextLocaleValue;
    }

    const std::string* Scalar::getLocalizedTextText() const {
        return d->stringValue;
    }

    void Scalar::setLocalizedText(const std::string* locale, const std::string* text,
            bool attachValues) {
        d->clear();
        d->scalarType = LOCALIZED_TEXT;
        d->localizedTextLocaleValue = locale;
        d->stringValue = text;
        d->hasAttachedValues = attachValues;
    }

    Variant* Scalar::copy() const {
        return new Scalar(*this);
    }

    Variant::Type Scalar::getVariantType() const {
        return SCALAR;
    }

    std::string Scalar::toString() const {
        std::ostringstream msg;
        msg << "IODataProviderNamespace::Scalar[type=";
        switch (d->scalarType) {
            case BOOL:
                msg << "bool,value=" << d->boolValue;
                break;
            case SCHAR:
                msg << "byte,value=" << d->scharValue;
                break;
            case CHAR:
                msg << "char,value=" << d->charValue;
                break;
            case INT:
                msg << "int,value=" << d->intValue;
                break;
            case UINT:
                msg << "uint,value=" << d->uintValue;
                break;
            case LONG:
                msg << "long,value=" << d->longValue;
                break;
            case ULONG:
                msg << "ulong,value=" << d->ulongValue;
                break;
            case LLONG:
                msg << "llong,value=" << d->llongValue;
                break;
            case ULLONG:
                msg << "ullong,value=" << d->ullongValue;
                break;
            case FLOAT:
                msg << "float,value=" << d->floatValue;
                break;
            case DOUBLE:
                msg << "double,value=" << d->doubleValue;
                break;
            case STRING:
                msg << "string,value=" << (d->stringValue == NULL ? "<NULL>" : *d->stringValue);
                break;
            case BYTE_STRING:
                msg << "byteString,value=";
                if (d->byteStringValue == NULL) {
                    msg << "<NULL>";
                } else {
                    msg << std::hex;
                    for (size_t i = 0; i < d->byteStringLengthValue; i++) {
                        if (i > 0) {
                            msg << ' ';
                        }
                        char s[11]; // 0xfffffffe
                        sprintf(s, "0x%x", d->byteStringValue[i]);
                        msg << s;
                    }
                }
                break;
            case LOCALIZED_TEXT:
                msg << "localizedText,locale=" << (d->localizedTextLocaleValue == NULL
                        ? "<NULL>" : *d->localizedTextLocaleValue)
                        << ",text=" << (d->stringValue == NULL ? "<NULL>" : *d->stringValue);
                break;
        }
        msg << "]";
        return msg.str();
    }

    void ScalarPrivate::clear() {
        if (hasAttachedValues) {
            switch (scalarType) {
                case Scalar::STRING:
                    delete stringValue;
                    break;
                case Scalar::BYTE_STRING:
                    delete[] byteStringValue;
                    break;
                case Scalar::LOCALIZED_TEXT:
                    delete localizedTextLocaleValue;
                    delete stringValue;
                    break;
                default:
                    break;
            }
        }
    }

} // namespace IODataProviderNamespace
