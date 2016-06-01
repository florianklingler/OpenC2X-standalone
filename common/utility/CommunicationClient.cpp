#include <utility/CommunicationClient.h>

#define REQUEST_TIMEOUT     2500    //  msecs, (> 1000!)
#define REQUEST_RETRIES     3       //  Before we abandon

CommunicationClient::CommunicationClient(string ownerModule, string portOut) {
	mOwnerModule = ownerModule;
	mPortOut = portOut;

	mContext = new zmq::context_t(1);
	mClient = NULL;
}

CommunicationClient::~CommunicationClient() {
	mContext->close();
	mClient->close();
	delete mContext;
	delete mClient;
	//delete mLogger;
}

void CommunicationClient::init() {
	mClient = new zmq::socket_t(*mContext, ZMQ_REQ);
	mClient->connect(("tcp://localhost:" + mPortOut).c_str());

	//mLogger = new LoggingUtility(mOwnerModule);

    //  Configure socket to not wait at close time
    int linger = 0;
    mClient->setsockopt (ZMQ_LINGER, &linger, sizeof (linger));
}



void CommunicationClient::sendRequest(string request) {
    int retries_left = REQUEST_RETRIES;
    std::cout << "I: request started" << std::endl;
    while (retries_left) {
        s_send (*mClient, request);
        std::cout << "I: request: (" << request << ")" << std::endl;
        sleep (1);

        bool expect_reply = true;
        while (expect_reply) {
            //  Poll socket for a reply, with timeout
            zmq::pollitem_t items[] = { { (void*) *mClient, 0, ZMQ_POLLIN, 0 } };
            zmq::poll (&items[0], 1, REQUEST_TIMEOUT);
            //  If we got a reply, process it
            if (items[0].revents & ZMQ_POLLIN) {
                //  We got a reply from the server, must match sequence
                std::string reply = s_recv (*mClient);
				std::cout << "I: server replied (" << reply << ")" << std::endl;
				retries_left = REQUEST_RETRIES;
				expect_reply = false;
            }
            else if (--retries_left == 0) {
                std::cout << "E: server seems to be offline, abandoning" << std::endl;
                expect_reply = false;
                break;
            }
            else {
                std::cout << "W: no response from server, retrying…" << std::endl;
                //  Old socket will be confused; close it and open a new one
                delete mClient;
                init();
                //  Send request again, on new socket
                s_send (*mClient, request);
            }
        }
    }
    delete mClient;

}
