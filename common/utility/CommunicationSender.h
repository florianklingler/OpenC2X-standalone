#ifndef UTILITY_COMMUNICATIONSENDER_H_
#define UTILITY_COMMUNICATIONSENDER_H_

#include <string>
#include <zmq.hpp>
#include <utility/zhelpers.hpp>
#include <utility/LoggingUtility.h>

class CommunicationSender {
public:
	CommunicationSender(std::string ownerModule, std::string portOut);
	~CommunicationSender();
	void send(std::string envelope, std::string message);
	void sendToHw(std::string message);
	void sendData(std::string envelope, std::string message);

private:
	std::string mOwnerModule;

	zmq::context_t* mContext;
	zmq::socket_t* mPublisher;

	LoggingUtility* mLogger;
};

#endif
