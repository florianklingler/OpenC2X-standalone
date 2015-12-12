#ifndef UTILITY_COMMUNICATIONRECEIVER_H_
#define UTILITY_COMMUNICATIONRECEIVER_H_

#include <string>
#include <zmq.hpp>
#include <utility/zhelpers.hpp>

using namespace std;

class CommunicationReceiver {
public:
	CommunicationReceiver(string portIn, string envelope);
	~CommunicationReceiver();
	pair<string, string> receive();

private:
	zmq::context_t* mContext;
	zmq::socket_t* mSubscriber;

	string mEnvelope;
};

#endif
