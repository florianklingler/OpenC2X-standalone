#include <utility/GpsDataSender.h>

GpsDataSender::GpsDataSender(string ownerModule, string portOut) {
	mOwnerModule = ownerModule;

	mContext = new zmq::context_t(1);
	mPublisher = new zmq::socket_t(*mContext, ZMQ_PUB);
	mPublisher->bind(("tcp://*:" + portOut).c_str());
}

GpsDataSender::~GpsDataSender() {
}

void GpsDataSender::send(string envelope, string message) {
	s_sendmore(*mPublisher, envelope);
	s_send(*mPublisher, message);
}
