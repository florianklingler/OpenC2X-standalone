#ifndef UTILITY_COMMUNICATIONCLIENT_H_
#define UTILITY_COMMUNICATIONCLIENT_H_

#include <string>
#include <zmq.hpp>
#include <utility/zhelpers.hpp>
#include <utility/LoggingUtility.h>

using namespace std;

class CommunicationClient {
public:
	CommunicationClient(string ownerModule, string portOut);
	~CommunicationClient();
	string sendRequest(string request, int timeout, int retries);
	void init();

private:
	string mOwnerModule;
	string mPortOut;

	zmq::context_t* mContext;
	zmq::socket_t* mClient;

	LoggingUtility* mLogger;
};

#endif
