#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE


#include "RecieveFromHarwareViaIP.h"
#include <string>
#include <arpa/inet.h>
#include <sstream>


RecieveFromHarwareViaIP::RecieveFromHarwareViaIP() {

	mSenderToDcc = new CommunicationSender("RecieveFromHardware", "4444");
	mLogger = new LoggingUtility("RecieveFromHardware");
}

RecieveFromHarwareViaIP::~RecieveFromHarwareViaIP() {
	mThreadReceive->join();
}

void RecieveFromHarwareViaIP::init(){
	/* create a UDP socket */

	if ((mSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		mLogger->logDebug("cannot create socket\n");
		return;
	}


	/* bind the socket to any valid IP address and a specific port */

	memset((char *)&mMyaddr, 0, sizeof(mMyaddr));
	mMyaddr.sin_family = AF_INET;
	mMyaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	mMyaddr.sin_port = htons(mService_port);

	if (bind(mSocket, (struct sockaddr *)&mMyaddr, sizeof(mMyaddr)) < 0) {
		mLogger->logDebug("bind failed");
		return;
	}



	mThreadReceive = new boost::thread(&RecieveFromHarwareViaIP::recieve, this);


}

void RecieveFromHarwareViaIP::recieve(){
	int seqno = 0;

	mLogger->logDebug("seqno\tdata\tport\trepetition\n");
	fflush(stdout);
	/* now loop, receiving data and printing what we received */
		while(true) {
			//fprintf(stderr, "waiting on port %d\n", service_port);
			mRecvlen = recvfrom(mSocket, mRecvBuffer, BUFSIZE, 0, (struct sockaddr *)&mRemoteAddr, &mAddrlen);
			//printf("received %d bytes\n", recvlen);
			if (mRecvlen > 0) {
				mRecvBuffer[mRecvlen] = 0;
				//printf("received message: \"%s\"\n", buf);

				std::ostringstream oss;
				oss <<  "\n seqno: " << seqno << " buf: "<< mRecvBuffer
						<< " serservice_port: " << mService_port
						<< " repetition: " << mRepetition << std::endl;
				mLogger->logDebug(oss.str());
				//send to dcc
				std::string tmp(reinterpret_cast<char*>(mRecvBuffer));
				mSenderToDcc->sendToHw(tmp);
			}
			seqno++;
		}
		/* never exits */
}
