#ifndef SASMODELPROVIDER_BASE_GENERATOR_GENERATORIODATAPROVIDER_H
#define SASMODELPROVIDER_BASE_GENERATOR_GENERATORIODATAPROVIDER_H

#include <ioDataProvider/MethodData.h>
#include <ioDataProvider/NodeData.h>
#include <ioDataProvider/NodeId.h>
#include <ioDataProvider/SubscriberCallback.h>
#include <sasModelProvider/base/ConverterUa2IO.h>
#include <sasModelProvider/base/NodeBrowser.h>
#include <vector>

namespace SASModelProviderNamespace {

    class GeneratorIODataProviderPrivate;

    class GeneratorIODataProvider {
    public:

        GeneratorIODataProvider(NodeBrowser& nodeBrowser, ConverterUa2IO& converter);
        virtual ~GeneratorIODataProvider();

        // The returned vector instance and its content must be destroyed by the caller.
        virtual std::vector<IODataProviderNamespace::NodeData*>* read(
                const std::vector<const IODataProviderNamespace::NodeId*>& nodeIds)
        /* throws ConversionException, DataGeneratorExceptionn */;
        // The returned vector instance and its content must be destroyed by the caller.
        virtual std::vector<IODataProviderNamespace::MethodData*>* call(
                std::vector<const IODataProviderNamespace::MethodData*>& methodDataList)
        /* throws ConversionException, DataGeneratorExceptionn */;
        virtual std::vector<IODataProviderNamespace::NodeData*>* subscribe(
                const std::vector<const IODataProviderNamespace::NodeId*>& nodeIds,
                IODataProviderNamespace::SubscriberCallback& callback);
    private:
        GeneratorIODataProvider(const GeneratorIODataProvider& orig);
        GeneratorIODataProvider& operator=(const GeneratorIODataProvider&);

        GeneratorIODataProviderPrivate* d;
    };
} // namespace SASModelProviderNamespace
#endif /* SASMODELPROVIDER_BASE_GENERATOR_GENERATORIODATAPROVIDER_H */

