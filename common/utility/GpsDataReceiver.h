#ifndef UTILITY_GPSDATARECEIVER_H_
#define UTILITY_GPSDATARECEIVER_H_

#include <string>
#include <zmq.hpp>
#include <utility/zhelpers.hpp>

using namespace std;

class GpsDataReceiver {
public:
	GpsDataReceiver(string ownerModule, string portIn, string envelope);
	~GpsDataReceiver();
	string receive();

private:
	string mOwnerModule;

	zmq::context_t* mContext;
	zmq::socket_t* mSubscriber;

	string mEnvelope;
};

#endif
