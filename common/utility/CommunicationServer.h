#ifndef UTILITY_COMMUNICATIONSERVER_H_
#define UTILITY_COMMUNICATIONSERVER_H_

#include <string>
#include <zmq.hpp>
#include "external/zhelpers.hpp"
#include "LoggingUtility.h"

/**
 * Replys to ZMQ requests from CommunicationClient.
 *
 * @ingroup communication
 */
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
