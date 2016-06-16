#ifndef UTILITY_COMMUNICATIONCLIENT_H_
#define UTILITY_COMMUNICATIONCLIENT_H_

#include <string>
#include <zmq.hpp>
#include "zhelpers.hpp"
#include "LoggingUtility.h"
#include <mutex>


class CommunicationClient {
public:
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
