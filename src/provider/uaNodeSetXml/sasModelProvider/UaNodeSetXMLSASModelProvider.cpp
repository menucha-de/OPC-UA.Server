#include "UaNodeSetXMLSASModelProvider.h"
#include "HaNodeManagerNodeSetXmlCreator.h"
#include "HaXmlUaNodeFactoryManager.h"
#include <common/Exception.h>
#include <common/logging/Logger.h>
#include <common/logging/LoggerFactory.h>
#include <sasModelProvider/SASModelProviderException.h>
#include <sasModelProvider/base/EventTypeRegistry.h>
#include <sasModelProvider/base/HaNodeManager.h>
#include <uastring.h> // UaString
#include <algorithm>  // std::sort
#include <dirent.h>
#include <sstream> // opendir

using namespace CommonNamespace;
using namespace SASModelProviderNamespace;

class UaNodeSetXMLSASModelProviderPrivate {
    friend class UaNodeSetXMLSASModelProvider;
private:
    Logger* log;

    IODataProviderNamespace::IODataProvider* ioDataProvider;
    std::vector<UaString> modelFiles;
    EventTypeRegistry eventTypeRegistry;
    // list of node managers (HaNodeManagerNodeSetXml) created by the server 
    // via the callback method HaNodeManagerNodeSetXmlCreator::createNodeManager
    // (the node managers are destroyed by the server)
    // The list is delegated to each node manager as associated node managers.
    std::vector<HaNodeManager*> nodeManagers;
    HaXmlUaNodeFactoryManagerSet uaNodeFactoryManagerSet;
};

UaNodeSetXMLSASModelProvider::UaNodeSetXMLSASModelProvider() {
    d = new UaNodeSetXMLSASModelProviderPrivate();
    d->log = LoggerFactory::getLogger("UaNodeSetXMLSASModelProvider");
}

UaNodeSetXMLSASModelProvider::~UaNodeSetXMLSASModelProvider() {
    delete d;
}

void UaNodeSetXMLSASModelProvider::open(std::string& confDir,
        IODataProviderNamespace::IODataProvider& ioDataProvider) /* throws SASModelProviderException */ {
    d->ioDataProvider = &ioDataProvider;
    UaString modelsDir(confDir.c_str());
    modelsDir += "models/";
    std::vector<std::string> dirEntries;
    DIR* dir;
    dirent* dirEntry;
    if ((dir = opendir(modelsDir.toUtf8())) != NULL) {
        /* get all files and directories within directory */
        while ((dirEntry = readdir(dir)) != NULL) {
            dirEntries.push_back(std::string(dirEntry->d_name));
        }
        closedir(dir);
    } else {
        std::ostringstream msg;
        msg << "Cannot open directory " << modelsDir.toUtf8();
        throw ExceptionDef(SASModelProviderException, msg.str());
    }
    std::sort(dirEntries.begin(), dirEntries.end());

    // for each directory entry of confDir
    for (std::vector<std::string>::const_iterator i = dirEntries.begin();
            i != dirEntries.end(); i++) {
        std::string dirEntry = *i;
        // the file must end with ".xml"
        if (!UaUniString(dirEntry.c_str()).endsWith(UaUniString(".xml"))) {
            continue;
        }
        UaString modelFile(modelsDir);
        modelFile += dirEntry.c_str();
        d->modelFiles.push_back(modelFile);
    }
}

void UaNodeSetXMLSASModelProvider::close() {
    d->modelFiles.clear();
}

std::vector<NodeManager*>* UaNodeSetXMLSASModelProvider::createNodeManagers() {
    return NULL;
}

std::vector<UaServerApplicationModule*>* UaNodeSetXMLSASModelProvider::createServerApplicationModules() {
    std::vector<UaServerApplicationModule*>* ret =
            new std::vector<UaServerApplicationModule*>();
    // for each directory entry of confDir
    for (int i = 0; i < d->modelFiles.size(); i++) {
        UaString& modelFile = d->modelFiles[i];
        d->log->info("Loading SAS model from %s", modelFile.toUtf8());
        // create a UaNode factory manager (creates methods and types itself 
        // and uses namespace dependent factories for creating variables and objects)
        HaXmlUaNodeFactoryManager* uaNodeFactoryManager = new HaXmlUaNodeFactoryManager();
        d->uaNodeFactoryManagerSet.add(*uaNodeFactoryManager);
        // create a node manager creator
        // A node manager adds a namespace dependent factory to all UaNode factory managers
        // at the end of its start up.
        HaNodeManagerNodeSetXmlCreator* nodeManagerCreator = new HaNodeManagerNodeSetXmlCreator(
                d->nodeManagers, d->eventTypeRegistry, d->uaNodeFactoryManagerSet,
                *d->ioDataProvider);
        // create app module / XML parser        
        ret->push_back(new UaNodeSetXmlParserUaNode(modelFile.toUtf8(),
                nodeManagerCreator, NULL /*baseNodeFactory*/, uaNodeFactoryManager));
    }
    return ret;
}
