#ifndef UTILITY_COMMUNICATIONSENDER_H_
#define UTILITY_COMMUNICATIONSENDER_H_

#define ELPP_NO_DEFAULT_LOG_FILE

#include <string>
#include <zmq.hpp>
#include <utility/zhelpers.hpp>
#include <utility/LoggingUtility.h>

using namespace std;

class CommunicationSender {
public:
	CommunicationSender(string ownerModule, string portOut);
	~CommunicationSender();
	void send(string envelope, string message);
	void sendToHw(string message);

private:
	string mOwnerModule;

	zmq::context_t* mContext;
	zmq::socket_t* mPublisher;

	LoggingUtility* mLogger;
};

#endif
