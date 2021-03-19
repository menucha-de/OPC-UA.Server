#ifndef BINARYSERVER_MESSAGEHANDLER_H
#define BINARYSERVER_MESSAGEHANDLER_H

#include <common/ModelType.h>

class MessageHandler {
    public: 
        MessageHandler() {};
        virtual ~MessageHandler() {};
        virtual void notificationReceived(Message& notification) = 0;
        virtual void eventReceived(Message& event) = 0;
        virtual void connectionStateChanged(int state) = 0;
        virtual void modelUpdated(std::map<std::string, std::map<std::string, ModelType> > newModel) = 0;
};
#endif
