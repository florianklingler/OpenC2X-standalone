#include "CommunicationReceiver.h"

CommunicationReceiver::CommunicationReceiver(string ownerModule, string portIn, string envelope) {
	mOwnerModule = ownerModule;

	mEnvelope = envelope;
	mContext = new zmq::context_t(1);
	mSubscriber = new zmq::socket_t(*mContext, ZMQ_SUB);
	mSubscriber->connect(("tcp://localhost:" + portIn).c_str());
	if (envelope == "") {
		mSubscriber->setsockopt(ZMQ_SUBSCRIBE, "", 0);//subscribe to all messages
	} else {
		mSubscriber->setsockopt(ZMQ_SUBSCRIBE, envelope.c_str(), 1);
	}

	mLogger = new LoggingUtility(mOwnerModule);
}

CommunicationReceiver::~CommunicationReceiver() {
	mContext->close();
	mSubscriber->close();
	delete mContext;
	delete mSubscriber;
	delete mLogger;
}

pair<string, string> CommunicationReceiver::receive() {
	string envelope = s_recv(*mSubscriber);
	string message = s_recv(*mSubscriber);

	mLogger->logDebug(envelope + " received");

	return make_pair(envelope, message);
}

string CommunicationReceiver::receiveFromHw() {
	string message = s_recv(*mSubscriber);

	mLogger->logDebug("received from HW");

	return message;
}

string CommunicationReceiver::receiveData() {
	string envelope = s_recv(*mSubscriber);
	string message = s_recv(*mSubscriber);
	return message;
}

