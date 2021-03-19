#include "Scalar.h"
#include <sstream> // std::ostringstream
#include <stdio.h>
#ifdef DEBUG
#include <CppUTest/MemoryLeakDetectorNewMacros.h>
#endif

class ScalarPrivate {
    friend class Scalar;
private:
    Scalar::Type scalarType;

    union {
        bool booleanValue;
        char charValue;
        signed char byteValue;
        int shortValue;
        long intValue;
        long long longValue;
        float floatValue;
        double doubleValue;
    };
};

Scalar::Scalar() {
    d = new ScalarPrivate();
    setInt(0);
}

Scalar::Scalar(const Scalar& orig) {
    d = new ScalarPrivate();
    d->scalarType = orig.d->scalarType;
    switch (d->scalarType) {
        case BOOLEAN:
            d->booleanValue = orig.d->booleanValue;
            break;
        case CHAR:
            d->charValue = orig.d->charValue;
            break;
        case BYTE:
            d->byteValue = orig.d->byteValue;
            break;
        case SHORT:
            d->shortValue = orig.d->shortValue;
            break;
        case INT:
            d->intValue = orig.d->intValue;
            break;
        case LONG:
            d->longValue = orig.d->longValue;
            break;
        case FLOAT:
            d->floatValue = orig.d->floatValue;
            break;
        case DOUBLE:
            d->doubleValue = orig.d->doubleValue;
            break;
    }
}

Scalar::~Scalar() {
    delete d;
}

Scalar::Type Scalar::getScalarType() const {
    return d->scalarType;
}

bool Scalar::getBoolean() const {
    return d->booleanValue;
}

void Scalar::setBoolean(bool value) {
    d->scalarType = BOOLEAN;
    d->booleanValue = value;
}

char Scalar::getChar() const {
    return d->charValue;
}

void Scalar::setChar(char value) {
    d->scalarType = CHAR;
    d->charValue = value;
}

signed char Scalar::getByte() const {
    return d->byteValue;
}

void Scalar::setByte(signed char value) {
    d->scalarType = BYTE;
    d->byteValue = value;
}

int Scalar::getShort() const {
    return d->shortValue;
}

void Scalar::setShort(int value) {
    d->scalarType = SHORT;
    d->shortValue = value;
}

long Scalar::getInt() const {
    return d->intValue;
}

void Scalar::setInt(long value) {
    d->scalarType = INT;
    d->intValue = value;
}

long long Scalar::getLong() const {
    return d->longValue;
}

void Scalar::setLong(long long value) {
    d->scalarType = LONG;
    d->longValue = value;
}

float Scalar::getFloat() const {
    return d->floatValue;
}

void Scalar::setFloat(float value) {
    d->scalarType = FLOAT;
    d->floatValue = value;
}

double Scalar::getDouble() const {
    return d->doubleValue;
}

void Scalar::setDouble(double value) {
    d->scalarType = DOUBLE;
    d->doubleValue = value;
}

Variant* Scalar::copy() const {
    return new Scalar(*this);
}

Variant::Type Scalar::getVariantType() const {
    return SCALAR;
}

std::string Scalar::toString() const {
    std::ostringstream msg;
    msg << "Scalar[type=";
    switch (d->scalarType) {
        case BOOLEAN:
            msg << "boolean,value=" << d->booleanValue;
            break;
        case CHAR:
            msg << "char,value=" << d->charValue;
            break;
        case BYTE:
            msg << "byte,value=" << d->byteValue;
            break;
        case SHORT:
            msg << "short,value=" << d->shortValue;
            break;
        case INT:
            msg << "int,value=" << d->intValue;
            break;
        case LONG:
            msg << "long,value=" << d->longValue;
            break;
        case FLOAT:
            msg << "float,value=" << d->floatValue;
            break;
        case DOUBLE:
            msg << "double,value=" << d->doubleValue;
            break;
    }
    msg << "]";
    return msg.str();
}
