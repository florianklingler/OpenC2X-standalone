#ifndef UTILITY_COMMUNICATIONSENDER_H_
#define UTILITY_COMMUNICATIONSENDER_H_

#include <string>
#include <zmq.hpp>
#include <utility/zhelpers.hpp>

using namespace std;

class CommunicationSender {
public:
	CommunicationSender(string portOut);
	~CommunicationSender();
	void send(string envelope, string message);

private:
	zmq::context_t* mContext;
	zmq::socket_t* mPublisher;
};

#endif
