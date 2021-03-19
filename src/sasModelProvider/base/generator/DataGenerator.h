#ifndef SASMODELPROVIDER_BASE_GENERATOR_DATAGENERATOR_H
#define SASMODELPROVIDER_BASE_GENERATOR_DATAGENERATOR_H

#include <ioDataProvider/OpcUaEventData.h>
#include <sasModelProvider/base/ConverterUa2IO.h>
#include <sasModelProvider/base/NodeBrowser.h>
#include <uaargument.h> // UaArgument
#include <uabasenodes.h> // UaVariables, UaObjectType
#include <uavariant.h> // UaVariant
#include <vector>

namespace SASModelProviderNamespace {

    class DataGeneratorPrivate;

    class DataGenerator {
    public:

        DataGenerator(NodeBrowser& nodeManager, ConverterUa2IO& converter);
        virtual ~DataGenerator();

        std::vector<UaVariant*>* generate(const std::vector<UaVariable*>& variables)
        /* throws ConversionException, DataGeneratorExceptionn */;
        std::vector<UaVariant*>* generate(const std::vector<UaArgument*>& arguments)
        /* throws ConversionException, DataGeneratorExceptionn */;
        std::vector<IODataProviderNamespace::OpcUaEventData*>* generate(
                const std::vector<UaObjectType*>& objectTypes)
        /* throws ConversionException, DataGeneratorExceptionn */;

    private:
        DataGenerator(const DataGenerator& orig);
        DataGenerator& operator=(const DataGenerator&);

        DataGeneratorPrivate* d;
    };
} // namespace SASModelProviderNamespace
#endif /* SASMODELPROVIDER_BASE_GENERATOR_DATAGENERATOR_H */

