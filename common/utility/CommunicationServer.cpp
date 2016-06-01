#include <utility/CommunicationServer.h>

CommunicationServer::CommunicationServer(string ownerModule, string portOut) {
	mOwnerModule = ownerModule;

	mContext = new zmq::context_t(1);
	mServer = new zmq::socket_t(*mContext, ZMQ_REP);
	mServer->bind(("tcp://*:" + portOut).c_str());

	//mLogger = new LoggingUtility(mOwnerModule);
}

CommunicationServer::~CommunicationServer() {
	mContext->close();
	mServer->close();
	delete mContext;
	delete mServer;
	//delete mLogger;
}


string CommunicationServer::receiveRequest() {
	string request = s_recv(*mServer);

	//mLogger->logDebug("received from HW");

	return request;
}

void CommunicationServer::sendReply(string reply) {
	//s_sendmore(*mReplyer, envelope);
	s_send(*mServer, reply);

	//mLogger->logDebug(envelope + " sent");
}
