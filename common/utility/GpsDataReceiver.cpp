#include "GpsDataReceiver.h"

GpsDataReceiver::GpsDataReceiver(string ownerModule, string portIn,
		string envelope) {
	mOwnerModule = ownerModule;

	mEnvelope = envelope;
	mContext = new zmq::context_t(1);
	mSubscriber = new zmq::socket_t(*mContext, ZMQ_SUB);
	mSubscriber->connect(("tcp://localhost:" + portIn).c_str());
	if (envelope == "") {
		mSubscriber->setsockopt(ZMQ_SUBSCRIBE, "", 0); //subscribe to all messages
	} else {
		mSubscriber->setsockopt(ZMQ_SUBSCRIBE, envelope.c_str(), 1);
	}

}

GpsDataReceiver::~GpsDataReceiver() {
}

string GpsDataReceiver::receive() {
	string envelope = s_recv(*mSubscriber);
	string message = s_recv(*mSubscriber);
	return message;
}

