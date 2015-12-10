#ifndef UTILITY_COMMUNICATIONRECEIVER_H_
#define UTILITY_COMMUNICATIONRECEIVER_H_

#include "ICommunication.h"
#include <zmq.hpp>
#include <string>
#include <utility/zhelpers.hpp>

using namespace std;

class CommunicationReceiver{
public:
	CommunicationReceiver(string portIn, string envelope);
	~CommunicationReceiver();
	virtual pair<string, string> receive();

private:
	zmq::context_t* mContext;
	zmq::socket_t* mSubscriber;

	string mEnvelope;

};

#endif
