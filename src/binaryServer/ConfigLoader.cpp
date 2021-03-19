#include "ConfigLoader.h"
#include <common/Exception.h>
#include <common/logging/Logger.h>
#include <common/logging/LoggerFactory.h>
#include <libxml/parser.h> // xmlReadFile
#include <libxml/encoding.h> // xmlGetCharEncodingName
#include <sstream> // std::ostringstream

using namespace CommonNamespace;

class ConfigLoaderPrivate {
    friend class ConfigLoader;
private:
    void parse(const xmlNode * startNode, ConfigLoader::ServerConfiguration& serverConf,
            ConfigLoader::RemoteConfiguration& sessionConf,
            ConfigLoader::LoggingConfiguration& returnLoggingConf);

    Logger* log;
    std::string confDir;
    std::string confFile;
    bool unitTesting;
};

ConfigLoader::ConfigLoader(std::string& confDir, std::string& confFile, bool unitTesting) {
    d = new ConfigLoaderPrivate();
    d->log = LoggerFactory::getLogger("ConfigLoader");
    d->confDir = confDir;
    d->confFile = confFile;
    d->unitTesting = unitTesting;
}

ConfigLoader::~ConfigLoader() {
    delete d;
}

void ConfigLoader::load(ServerConfiguration& returnServerConf, RemoteConfiguration& returnRemoteConf,
        LoggingConfiguration& returnLoggingConf) {
    // set default values
    returnServerConf.port = 4223;
    returnServerConf.acceptTimeout = 60; // in sec.
    returnServerConf.sendReceiveTimeout = 30; // in sec.    

    returnRemoteConf.host = "127.0.0.1";
    returnRemoteConf.port = 4810;
    returnRemoteConf.username = NULL;
    returnRemoteConf.password = NULL;
    returnRemoteConf.connectTimeout = 120; // sec.
    returnRemoteConf.sendReceiveTimeout = 30; // sec.
    returnRemoteConf.maxReconnectDelay = 10; // sec.
    returnRemoteConf.publishingInterval = 100; // ms

    returnLoggingConf.uaStackLogLevel = "NONE";
    returnLoggingConf.uaAppLogLevel = "INFO";
    // assuming a sub dir like "conf" contains this config file
    returnLoggingConf.filePath = std::string(d->confDir).append("../var/log/uaclient.log");
    returnLoggingConf.maxNumBackupFiles = 5;

    xmlInitParser();
    std::string confFilePath(d->confDir);
    confFilePath.append(d->confFile);
    xmlDoc* doc = xmlReadFile(confFilePath.c_str(), NULL /*encoding*/, 0 /* options */);
    if (doc == NULL) {
        xmlCleanupParser();
        std::ostringstream msg;
        msg << "Cannot parse configuration file";
        //TODO ConfigurationException
        throw ExceptionDef(Exception, msg.str());
    }
    d->parse(xmlDocGetRootElement(doc), returnServerConf, returnRemoteConf,
            returnLoggingConf);
    xmlFreeDoc(doc);
    xmlCleanupParser();
}

void ConfigLoaderPrivate::parse(const xmlNode * startNode,
        ConfigLoader::ServerConfiguration& returnServerConf,
        ConfigLoader::RemoteConfiguration& returnRemoteConf,
        ConfigLoader::LoggingConfiguration& returnLoggingConf) {
    xmlChar* server = xmlCharStrdup("server");
    xmlChar* serverPort = xmlCharStrdup("port");
    xmlChar* acceptTimeout = xmlCharStrdup("acceptTimeout");
    xmlChar* serverSendReceiveTimeout = xmlCharStrdup("sendReceiveTimeout");

    xmlChar* remote = xmlCharStrdup("remote");
    xmlChar* host = xmlCharStrdup("host");
    xmlChar* port = xmlCharStrdup("port");
    xmlChar* username = xmlCharStrdup("username");
    xmlChar* password = xmlCharStrdup("password");
    xmlChar* connectTimeout = xmlCharStrdup("connectTimeout");
    xmlChar* sendReceiveTimeout = xmlCharStrdup("sendReceiveTimeout");
    xmlChar* maxReconnectDelay = xmlCharStrdup("maxReconnectDelay");
    xmlChar* publishingInterval = xmlCharStrdup("publishingInterval");

    xmlChar* logging = xmlCharStrdup("logging");
    xmlChar* uaStackLogLevel = xmlCharStrdup("uaStackLogLevel");
    xmlChar* uaAppLogLevel = xmlCharStrdup("uaAppLogLevel");
    xmlChar* filePath = xmlCharStrdup("filePath");
    xmlChar* maxNumBackupFiles = xmlCharStrdup("maxNumBackupFiles");

    for (const xmlNode* currNode = startNode; currNode != NULL; currNode =
            currNode->next) {
        if (currNode->type == XML_ELEMENT_NODE) {
            // server
            if (xmlStrEqual(currNode->name, server) != 0) {
                xmlAttr* props = currNode->properties;
                if (props != NULL) {
                    for (xmlAttr* prop = props; prop != NULL; prop =
                            prop->next) {
                        if (currNode->type == XML_ELEMENT_NODE) {
                            if (xmlStrEqual(prop->name, serverPort) != 0) {
                                returnServerConf.port = atoi((char *) prop->children->content);
                            } else if (xmlStrEqual(prop->name, acceptTimeout) != 0) {
                                returnServerConf.acceptTimeout = atoi((char *) prop->children->content);
                            } else if (xmlStrEqual(prop->name, serverSendReceiveTimeout) != 0) {
                                returnServerConf.sendReceiveTimeout = atoi(
                                        (char *) prop->children->content);
                            } else {
                                log->warn("Unknown configuration attribute found for element '%s': %s",
                                        currNode->name, prop->name);
                            }
                        }
                    }
                }
                // remote
            } else if (xmlStrEqual(currNode->name, remote) != 0) {
                xmlAttr* props = currNode->properties;
                if (props != NULL) {
                    for (xmlAttr* prop = props; prop != NULL; prop = prop->next) {
                        if (currNode->type == XML_ELEMENT_NODE) {
                            if (xmlStrEqual(prop->name, host) != 0) {
                                returnRemoteConf.host = std::string(
                                        (char*) prop->children->content);
                            } else if (xmlStrEqual(prop->name, port) != 0) {
                                returnRemoteConf.port = atoi(
                                        (char *) prop->children->content);
                            } else if (xmlStrEqual(prop->name, username) != 0) {
                                returnRemoteConf.username = new std::string(
                                        (char*) prop->children->content);
                            } else if (xmlStrEqual(prop->name, password) != 0) {
                                returnRemoteConf.password = new std::string(
                                        (char*) prop->children->content);
                            } else if (xmlStrEqual(prop->name, connectTimeout) != 0) {
                                returnRemoteConf.connectTimeout = atoi(
                                        (char *) prop->children->content);
                            } else if (xmlStrEqual(prop->name, sendReceiveTimeout) != 0) {
                                returnRemoteConf.sendReceiveTimeout = atoi(
                                        (char *) prop->children->content);
                            } else if (xmlStrEqual(prop->name, maxReconnectDelay) != 0) {
                                returnRemoteConf.maxReconnectDelay = atoi(
                                        (char *) prop->children->content);
                            } else if (xmlStrEqual(prop->name, publishingInterval) != 0) {
                                returnRemoteConf.publishingInterval = atoi(
                                        (char *) prop->children->content);
                            } else {
                                log->warn("Unknown configuration attribute found for element '%s': %s",
                                        currNode->name, prop->name);
                            }
                        }
                    }
                }
                // logging
            } else if (xmlStrEqual(currNode->name, logging) != 0) {
                xmlAttr* props = currNode->properties;
                if (props != NULL) {
                    for (xmlAttr* prop = props; prop != NULL; prop = prop->next) {
                        if (currNode->type == XML_ELEMENT_NODE) {
                            if (xmlStrEqual(prop->name, uaStackLogLevel) != 0) {
                                returnLoggingConf.uaStackLogLevel = std::string(
                                        (char*) prop->children->content);
                            } else if (xmlStrEqual(prop->name, uaAppLogLevel) != 0) {
                                returnLoggingConf.uaAppLogLevel = std::string(
                                        (char*) prop->children->content);
                            } else if (xmlStrEqual(prop->name, filePath) != 0) {
                                returnLoggingConf.filePath = std::string(
                                        (char*) prop->children->content);
                                // if relative path
                                if (returnLoggingConf.filePath.length() > 0 &&
                                        '/' != returnLoggingConf.filePath.at(0)) {
                                    // convert to absolute path
                                    returnLoggingConf.filePath = std::string(confDir)
                                            .append(returnLoggingConf.filePath);
                                }
                            } else if (xmlStrEqual(prop->name, maxNumBackupFiles) != 0) {
                                returnLoggingConf.maxNumBackupFiles = atoi(
                                        (char *) prop->children->content);
                            } else {
                                log->warn("Unknown configuration attribute found for element '%s': %s",
                                        currNode->name, prop->name);
                            }
                        }
                    }
                }
            }
            parse(currNode->children, returnServerConf, returnRemoteConf, returnLoggingConf);
        }
    }
    // usage of libxml2: "Deallocating non-allocated memory"
    if (!unitTesting) {
        delete[] server;
        delete[] serverPort;
        delete[] acceptTimeout;
        delete[] serverSendReceiveTimeout;

        delete[] remote;
        delete[] port;
        delete[] username;
        delete[] password;
        delete[] host;
        delete[] connectTimeout;
        delete[] sendReceiveTimeout;
        delete[] maxReconnectDelay;
        delete[] publishingInterval;

        delete[] logging;
        delete[] uaStackLogLevel;
        delete[] uaAppLogLevel;
        delete[] filePath;
        delete[] maxNumBackupFiles;
    }
}