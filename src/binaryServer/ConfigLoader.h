#ifndef BINARYSERVER_CONFIGLOADER_H
#define BINARYSERVER_CONFIGLOADER_H

#include <string>

class ConfigLoaderPrivate;

class ConfigLoader {
public:

    class ServerConfiguration {
    public:
        int port;
        int acceptTimeout; // in sec.
        int sendReceiveTimeout; // in sec.    
    };

    class RemoteConfiguration {
    public:
        std::string host;
        int port;
        std::string* username;
        std::string* password;
        int connectTimeout;
        int sendReceiveTimeout;
        int maxReconnectDelay;
        int publishingInterval;
    };

    class LoggingConfiguration {
    public:
        std::string uaStackLogLevel;
        std::string uaAppLogLevel;
        std::string filePath;
        int maxNumBackupFiles;
    };

    ConfigLoader(std::string& confDir, std::string& confFile, bool unitTesting = false);
    virtual ~ConfigLoader();

    virtual void load(ServerConfiguration& returnServerConf,
            RemoteConfiguration& returnRemoteConf, LoggingConfiguration& returnLoggingConf);
private:
    ConfigLoader(const ConfigLoader& orig);
    ConfigLoader& operator=(const ConfigLoader&);

    ConfigLoaderPrivate* d;
};

#endif /* BINARYSERVER_CONFIGLOADER_H */

