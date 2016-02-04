
#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE


#include "ReceiveFromHardwareViaIP.h"
#include "dcc.h"
#include <string>
#include <arpa/inet.h>
#include <sstream>


ReceiveFromHardwareViaIP::ReceiveFromHardwareViaIP(DCC* dcc) {
	mOwner = dcc;
	mLogger = new LoggingUtility("ReceiveFromHardware");
}

ReceiveFromHardwareViaIP::~ReceiveFromHardwareViaIP() {
	mThreadReceive->join();
	delete mThreadReceive;

	delete mLogger;
}

void ReceiveFromHardwareViaIP::init(){
	/* create a UDP socket */

	if ((mSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		mLogger->logDebug("cannot create socket");
		return;
	}


	/* bind the socket to any valid IP address and a specific port */

	memset((char *)&mMyAddr, 0, sizeof(mMyAddr));
	mMyAddr.sin_family = AF_INET;
	mMyAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	mMyAddr.sin_port = htons(mServicePort);

	if (bind(mSocket, (struct sockaddr *)&mMyAddr, sizeof(mMyAddr)) < 0) {
		mLogger->logDebug("bind failed");
		return;
	}

	mThreadReceive = new boost::thread(&ReceiveFromHardwareViaIP::receive, this);
}

void ReceiveFromHardwareViaIP::receive(){
	int seqno = 0;

	fflush(stdout);
	/* now loop, receiving data and printing what we received */
	while(true) {
		//fprintf(stderr, "waiting on port %d\n", service_port);
		mRecvLen = recvfrom(mSocket, mRecvBuffer, BUFSIZE, 0, (struct sockaddr *)&mRemoteAddr, &mAddrLen);
		//printf("received %d bytes\n", recvlen);
		if (mRecvLen > 0) {
			mRecvBuffer[mRecvLen] = 0;
			//printf("received message: \"%s\"\n", buf);
			//cout << "Recv: len: "<< mRecvlen << " raw: " << mRecvBuffer << endl;
			mLogger->logDebug("Sequence no: " + std::to_string(seqno) + "\tService port: " + std::to_string(mServicePort) + "\tRepetition: " + std::to_string(mRepetition) + "\tLength: " + std::to_string(mRecvLen));

			//send to dcc
			std::string tmp(reinterpret_cast<char*>(mRecvBuffer), mRecvLen);
			mOwner->receiveFromHw(tmp);
		}
		seqno++;
	}
	/* never exits */
}
