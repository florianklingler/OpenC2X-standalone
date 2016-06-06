#ifndef UTILITY_COMMUNICATIONSERVER_H_
#define UTILITY_COMMUNICATIONSERVER_H_

#include <string>
#include <zmq.hpp>
#include <utility/zhelpers.hpp>
#include <utility/LoggingUtility.h>

using namespace std;

class CommunicationServer {
public:
	CommunicationServer(string ownerModule, string portOut);
	~CommunicationServer();
	void sendReply(string reply);
	string receiveRequest();

private:
	string mOwnerModule;

	zmq::context_t* mContext;
	zmq::socket_t* mServer;

	LoggingUtility* mLogger;
};

#endif
