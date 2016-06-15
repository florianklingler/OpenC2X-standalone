#ifndef UTILITY_COMMUNICATIONSERVER_H_
#define UTILITY_COMMUNICATIONSERVER_H_

#include <string>
#include <zmq.hpp>
#include "zhelpers.hpp"
#include "LoggingUtility.h"

class CommunicationServer {
public:
	CommunicationServer(std::string ownerModule, std::string portOut, int expNo);
	~CommunicationServer();
	void sendReply(std::string reply);
	std::pair<std::string, std::string> receiveRequest();

private:
	std::string mOwnerModule;

	zmq::context_t* mContext;
	zmq::socket_t* mServer;

	LoggingUtility* mLogger;
};

#endif
