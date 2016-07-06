#ifndef UTILITY_COMMUNICATIONSENDER_H_
#define UTILITY_COMMUNICATIONSENDER_H_

#include <string>
#include <zmq.hpp>
#include "external/zhelpers.hpp"
#include "LoggingUtility.h"


/**
 * Sender of zmq broadcasts to be received by CommunicationReceiver.
 *
 * @ingroup communication
 */
class CommunicationSender {
public:
	/**
	 * ownerModule and expNo forwarded to LoggingUtility constructor
	 * @param ownerModule Module Name
	 * @param expNo Experiment Number
	 * @param portOut port used for broadcasting
	 */
	CommunicationSender(std::string ownerModule, std::string portOut, int expNo);
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
