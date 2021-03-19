#ifndef BINARYSERVER_CLIENT_H
#define BINARYSERVER_CLIENT_H

#include <string>
#include "../provider/binary/messages/dto/ReadResponse.h"
#include "../provider/binary/messages/dto/CallResponse.h"
#include "../provider/binary/messages/dto/Read.h"
#include "../provider/binary/messages/dto/Call.h"
#include "../provider/binary/messages/dto/Write.h"
#include "../provider/binary/messages/dto/Subscribe.h"
#include "../provider/binary/messages/dto/Unsubscribe.h"
#include <common/logging/Logger.h>
#include <map>
#include "../common/native2J/MessageHandler.h"
#include <uanodeid.h> // UaNodeId
#include "../../include/common/ModelType.h"

class ClientPrivate;

class Client {
public:

	class Options {
	public:
		int* serverPort = NULL;
		int* publishInterval = NULL;
		int* connectTimeout = NULL;
		int* sendReceiveTimeout = NULL;
		int* watchdogInterval = NULL;
		std::string* remoteHost = NULL;
		int* remotePort = NULL;
		std::string* username = NULL;
		std::string* password = NULL;
		std::string* loggingFilePath = NULL;
	};

	Client() /* throws MutexException */;
	virtual ~Client() /* throws HaSessionException, HaSubscriptionException */;

	virtual void open(Options& conf,
			MessageHandler *handler) /* throws ServerException, ServerSocketException, HaSessionException */;
	virtual void close() /* throws HaSessionException, HaSubscriptionException */;
	virtual ReadResponse read(Read &read);
	//TODO Subscribe Response
	virtual void subscribe(Subscribe &subscribe);
	//TODO Unsubscribe Response
	virtual void unsubscribe(Unsubscribe &unsubscribe);
	//TODO Write Response
	virtual void write(Write &write);

	virtual CallResponse call(Call &call);

	virtual std::map<std::string, std::map<std::string, std::string> > browse(int ns, const std::string startNode, const std::string prefix);
	virtual std::map<std::string, std::map<std::string, std::string> > browse(int ns, int startNode, const std::string prefix);

	virtual std::string getSessionId();

	virtual std::vector<std::string> checkModel(const ParamId& pId, int type);


private:

	Client(const Client& orig);
	Client& operator=(const Client&);

	ClientPrivate* d;
};

#endif /* BINARYSERVER_CLIENT_H */

