#include "CommunicationClient.h"

using namespace std;

CommunicationClient::CommunicationClient(string ownerModule, string portOut, int expNo) {
	mOwnerModule = ownerModule;
	mPortOut = portOut;

	mLogger = new LoggingUtility(mOwnerModule, expNo);

	mContext = new zmq::context_t(1);
	mClient = NULL;
}

CommunicationClient::~CommunicationClient() {
	mContext->close();
	mClient->close();
	delete mContext;
	delete mClient;
	delete mLogger;
}

void CommunicationClient::init() {
	mClient = new zmq::socket_t(*mContext, ZMQ_REQ);
	mClient->connect(("tcp://localhost:" + mPortOut).c_str());

    //  Configure socket to not wait at close time
    int linger = 0;
    mClient->setsockopt (ZMQ_LINGER, &linger, sizeof (linger));
}


//timeout in ms
string CommunicationClient::sendRequest(string envelope, string request, int timeout) {
	mMutex.lock();
	init();
	s_sendmore(*mClient, envelope);
	s_send (*mClient, request);
	mLogger->logDebug("sent request to ldm: " + envelope + ", " + request);
	//Poll socket for a reply, with timeout
	zmq::pollitem_t items[] = { { (void*) *mClient, 0, ZMQ_POLLIN, 0 } };
	zmq::poll (&items[0], 1, timeout);
	//If we got a reply, process it
	if (items[0].revents & ZMQ_POLLIN) {
		//  We got a reply from the server, return it to requester
		std::string reply = s_recv (*mClient);
		mLogger->logDebug("ldm replied");
	    delete mClient;
	    mMutex.unlock();
		return reply;
	}
    //could not get a reply, return ""
	mLogger->logDebug("no response from ldm");
    delete mClient;
    mMutex.unlock();
    return "";
}
