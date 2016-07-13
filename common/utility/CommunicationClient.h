#ifndef UTILITY_COMMUNICATIONCLIENT_H_
#define UTILITY_COMMUNICATIONCLIENT_H_

#include <string>
#include <zmq.hpp>
#include "external/zhelpers.hpp"
#include "LoggingUtility.h"
#include <mutex>

/**
 * Send ZMQ request to CommunicationServer and waits a specified time for an answer.
 *
 * @ingroup communication
 */
class CommunicationClient {
public:
	/**
	 * ownerModule and expNo forwarded to LoggingUtility constructor
	 * @param ownerModule Module Name
	 * @param expNo Experiment Number
	 * @param portOut port used for requesting
	 */
	CommunicationClient(std::string ownerModule, std::string portOut, int expNo);
	~CommunicationClient();
	std::string sendRequest(std::string envelope, std::string request, int timeout);
	void init();

private:
	std::string mOwnerModule;
	std::string mPortOut;

	zmq::context_t* mContext;
	zmq::socket_t* mClient;

	LoggingUtility* mLogger;

	std::mutex mMutex;
};

#endif
