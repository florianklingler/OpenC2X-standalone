#include <utility/CommunicationSender.h>

CommunicationSender::CommunicationSender(string ownerModule, string portOut) {
	mOwnerModule = ownerModule;

	mContext = new zmq::context_t(1);
	mPublisher = new zmq::socket_t(*mContext, ZMQ_PUB);
	mPublisher->bind(("tcp://*:" + portOut).c_str());

	mLogger = new LoggingUtility(mOwnerModule);
}

CommunicationSender::~CommunicationSender() {
}

void CommunicationSender::send(string envelope, string message) {
	s_sendmore(*mPublisher, envelope);
	s_send(*mPublisher, message);

	mLogger->logDebug(envelope + " sent");
}

void CommunicationSender::sendToHw(string message) {
	s_send(*mPublisher, message);

	mLogger->logDebug("sent to HW");
}
