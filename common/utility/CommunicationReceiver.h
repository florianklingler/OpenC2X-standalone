#ifndef UTILITY_COMMUNICATIONRECEIVER_H_
#define UTILITY_COMMUNICATIONRECEIVER_H_

#include <string>
#include <zmq.hpp>
#include "external/zhelpers.hpp"
#include "LoggingUtility.h"


/**
 * Receiver of zmq broadcasts send by CommunicationSender.
 *
 * @ingroup communication
 */
class CommunicationReceiver {
public:
	/**
	 * ownerModule and expNo forwarded to LoggingUtility constructor
	 * @param ownerModule Module Name
	 * @param expNo Experiment Number
	 * @param portIn port to listen on for broadcasts
	 */
	CommunicationReceiver(std::string ownerModule, std::string portIn, std::string envelope, int expNo);
	~CommunicationReceiver();
	std::pair<std::string, std::string> receive();
	std::string receiveFromHw();
	std::string receiveData();

private:
	std::string mOwnerModule;

	zmq::context_t* mContext;
	zmq::socket_t* mSubscriber;

	LoggingUtility* mLogger;

	std::string mEnvelope;
};

#endif
