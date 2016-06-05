#include <utility/CommunicationClient.h>

CommunicationClient::CommunicationClient(string ownerModule, string portOut) {
	mOwnerModule = ownerModule;
	mPortOut = portOut;

	mLogger = new LoggingUtility(mOwnerModule);

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


//timeout in ms (>1000)
string CommunicationClient::sendRequest(string request, int timeout, int retries) {
	init();
    int retries_left = retries;
    while (retries_left) {
        s_send (*mClient, request);
        mLogger->logDebug("sent request to ldm: " + request);

        bool expect_reply = true;
        while (expect_reply) {
            //  Poll socket for a reply, with timeout
            zmq::pollitem_t items[] = { { (void*) *mClient, 0, ZMQ_POLLIN, 0 } };
            zmq::poll (&items[0], 1, timeout);
            //  If we got a reply, process it
            if (items[0].revents & ZMQ_POLLIN) {
                //  We got a reply from the server, return it to requester
                std::string reply = s_recv (*mClient);
                mLogger->logDebug("ldm replied: " + reply);
				retries_left = 0;
				expect_reply = false;
				return reply;
            } else if (--retries_left == 0) {
                mLogger->logDebug("ldm seems to be offline, abandoning request");
                expect_reply = false;
                break;
			} else {
                mLogger->logDebug("no response from ldm, retrying…");
                //  Old socket will be confused; close it and open a new one
                delete mClient;
                init();
                //  Send request again, on new socket
                s_send (*mClient, request);
			}
		}
	}
    //could not get a reply, return ""
    delete mClient;
    return "";
}
