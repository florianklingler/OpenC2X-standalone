#ifndef UTILITY_COMMUNICATIONRECEIVER_H_
#define UTILITY_COMMUNICATIONRECEIVER_H_

#define ELPP_NO_DEFAULT_LOG_FILE

#include <string>
#include <zmq.hpp>
#include <utility/zhelpers.hpp>
#include <utility/LoggingUtility.h>

using namespace std;

class CommunicationReceiver {
public:
	CommunicationReceiver(string portIn, string envelope);
	~CommunicationReceiver();
	pair<string, string> receive();
	string receiveFromHw();

private:
	zmq::context_t* mContext;
	zmq::socket_t* mSubscriber;

	LoggingUtility* mLogger;

	string mEnvelope;
};

#endif
