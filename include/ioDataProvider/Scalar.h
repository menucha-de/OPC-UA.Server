#ifndef IODATAPROVIDER_SCALAR_H_
#define IODATAPROVIDER_SCALAR_H_

#include "Variant.h"
#include <string>

namespace IODataProviderNamespace {

    class ScalarPrivate;

    class Scalar : public Variant {
    public:

        enum Type {
            BOOL = 1, SCHAR = 2, CHAR = 3, INT = 4, UINT = 5, LONG = 6, ULONG = 7, LLONG = 8,
            ULLONG = 9, FLOAT = 10, DOUBLE = 11, STRING = 12,
            BYTE_STRING = 15, LOCALIZED_TEXT = 21
        };

        // INT with value 0        
        Scalar();
        // Creates a deep copy of the instance.
        Scalar(const Scalar& value);
        virtual ~Scalar();

        virtual Type getScalarType() const;

        virtual bool getBool() const;
        virtual void setBool(bool value);

        // 8 bits UTF-8
        virtual char getChar() const;
        virtual void setChar(char value);

        // 8 bits
        virtual signed char getSChar() const;
        virtual void setSChar(signed char value);

        // 16 bits
        virtual int getInt() const;
        virtual void setInt(int value);

        // 16 bits
        virtual unsigned int getUInt() const;
        virtual void setUInt(unsigned int value);

        // 32 bits
        virtual long getLong() const;
        virtual void setLong(long value);

        // 32 bits
        virtual unsigned long getULong() const;
        virtual void setULong(unsigned long value);

        // 64bits
        virtual long long getLLong() const;
        virtual void setLLong(long long value);

        // 64bits
        virtual unsigned long long getULLong() const;
        virtual void setULLong(unsigned long long value);

        virtual float getFloat() const;
        virtual void setFloat(float value);

        virtual double getDouble() const;
        virtual void setDouble(double value);

        // UTF-8 encoded        
        virtual const std::string* getString() const;
        // A string may be NULL (see OpcUa Part 6 5.2.2.4).
        // If the values are attached then the responsibility for destroying the value instances
        // is delegated to the Scalar instance.
        virtual void setString(const std::string* value, bool attachValue = false);

        virtual const char* getByteString() const;
        virtual long getByteStringLength() const;
        // A byte string may be NULL (see OpcUa Part 6 5.2.2.6).
        // If the values are attached then the responsibility for destroying the value instances
        // is delegated to the Scalar instance.
        virtual void setByteString(const char* value, long length, bool attachValue = false);

        virtual const std::string* getLocalizedTextLocale() const;
        virtual const std::string* getLocalizedTextText() const;
        // The fields of a localized text may be NULL (see OpcUa Part 6 5.2.2.14).
        // If the values are attached then the responsibility for destroying the value instances
        // is delegated to the Scalar instance.
        virtual void setLocalizedText(const std::string* locale, const std::string* text,
                bool attachValues = false);

        // interface Variant
        virtual Variant* copy() const;
        virtual Variant::Type getVariantType() const;
        virtual std::string toString() const;
    private:
        Scalar& operator=(const Scalar&);

        ScalarPrivate* d;
    };

} // namespace IODataProviderNamespace
#endif /* IODATAPROVIDER_SCALAR_H_ */
