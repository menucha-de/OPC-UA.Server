#ifndef PROVIDER_BINARY_MESSAGES_DTO_SCALAR_H
#define PROVIDER_BINARY_MESSAGES_DTO_SCALAR_H

#include "Variant.h"
#include <string>

class ScalarPrivate;

class Scalar : public Variant {
public:

    enum Type {
        BOOLEAN = 0, //
        CHAR = 1,
        BYTE = 2,
        SHORT = 3,
        INT = 4,
        LONG = 5,
        FLOAT = 6,
        DOUBLE = 7
    };

    // INT with value 0
    Scalar();
    // Creates a deep copy of the Scalar instance.
    Scalar(const Scalar& value);
    virtual ~Scalar();

    virtual Type getScalarType() const;

    // binary representation: 8 bits
    virtual bool getBoolean() const;
    virtual void setBoolean(bool value);

    // 8 bits UTF-8
    virtual char getChar() const;
    virtual void setChar(char value);

    // 8 bits
    virtual signed char getByte() const;
    virtual void setByte(signed char value);

    // 16 bits
    virtual int getShort() const;
    virtual void setShort(int value);

    // 32 bits
    virtual long getInt() const;
    virtual void setInt(long value);

    // 64 bit: value 1 with bits 31-16, value2 with bits 15-0
    long long getLong() const;
    void setLong(long long value);

    // 32 bits
    virtual float getFloat() const;
    virtual void setFloat(float value);

    // 64 bits
    virtual double getDouble() const;
    virtual void setDouble(double value);

    // interface Variant
    virtual Variant* copy() const;
    virtual Variant::Type getVariantType() const;
    virtual std::string toString() const;
private:
    Scalar& operator=(const Scalar& orig);

    ScalarPrivate* d;
};

#endif /* PROVIDER_BINARY_MESSAGES_DTO_SCALAR_H */
