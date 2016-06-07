#include <utility/CommunicationServer.h>

CommunicationServer::CommunicationServer(string ownerModule, string portOut) {
	mOwnerModule = ownerModule;

	mContext = new zmq::context_t(1);
	mServer = new zmq::socket_t(*mContext, ZMQ_REP);
	mServer->bind(("tcp://*:" + portOut).c_str());

	mLogger = new LoggingUtility(mOwnerModule);
}

CommunicationServer::~CommunicationServer() {
	mContext->close();
	mServer->close();
	delete mContext;
	delete mServer;
	delete mLogger;
}


pair<string, string> CommunicationServer::receiveRequest() {
	string envelope = s_recv(*mServer);
	string request = s_recv(*mServer);

	mLogger->logDebug("received request: " + envelope + ", " + request);

	return make_pair(envelope, request);
}

void CommunicationServer::sendReply(string reply) {
	//s_sendmore(*mReplyer, envelope);	//TODO: do we need an envelope here?
	s_send(*mServer, reply);

	mLogger->logDebug("sent reply: " + reply);
}
