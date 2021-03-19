#ifndef SERVER_H
#define SERVER_H

#include <ioDataProvider/IODataProviderFactory.h>
#include <sasModelProvider/SASModelProviderFactory.h>
#include <map>
#include <string>
#include <jni.h>
#include <jni_md.h>

class ServerPrivate;

class Server {
public:
    // shutdownDelay: allow clients to disconnect after they received the shutdown signal 
    //                (in seconds)
    Server(OpcUa_UInt shutdownDelay);
    ~Server();

    void open() /*throws ServerException, IODataProviderException, SASModelProviderException*/;
    void open(JNIEnv *env, jobject properties, jobject dataProvider) /*throws ServerException, IODataProviderException, SASModelProviderException*/;

    void notification(JNIEnv *env, int ns, jobject id, jobject msg);
    void event(JNIEnv *env, int eNs, jobject event, int pNs, jobject param, long timestamp, int severity, jstring msg, jobject value);

    void close();



private:
    ServerPrivate* d;
};

#endif /* SERVER_H */

