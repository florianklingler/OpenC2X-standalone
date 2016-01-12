#ifndef UTILITY_GPSDATASENDER_H_
#define UTILITY_GPSDATASENDER_H_

#include <string>
#include <zmq.hpp>
#include <utility/zhelpers.hpp>

using namespace std;

class GpsDataSender {
public:
	GpsDataSender(string ownerModule, string portOut);
	~GpsDataSender();
	void send(string envelope, string message);

private:
	string mOwnerModule;

	zmq::context_t* mContext;
	zmq::socket_t* mPublisher;

	string mEnvelope;
};

#endif
