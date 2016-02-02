#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include "SendToHardwareViaIP.h"
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>

using namespace std;

SendToHardwareViaIP::SendToHardwareViaIP() {
	mLogger = new LoggingUtility("SendToHardware");
	//TODO: change this IP, current value is for testing on my virtual machine
	mServer = "127.0.0.255";
}

SendToHardwareViaIP::~SendToHardwareViaIP() {
	close(mSocket);
}

bool SendToHardwareViaIP::init() {
	if ((mSocket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		mLogger->logDebug("Cannot create socket\n");
		cout << "Cannot create socket\n";
		return 0;
	}

	// Enable broadcasting
	int broadcastEnable = 1;
	int ret = setsockopt(mSocket, SOL_SOCKET, SO_BROADCAST, &broadcastEnable,
			sizeof(broadcastEnable));
	if (ret != 0) {
		mLogger->logDebug("Error while enabling broadcasting\n");
		cout << "Error while enabling broadcasting\n";
	}

	// set the flags
	int flags = fcntl(mSocket, F_GETFL, 0);
	fcntl(mSocket, F_SETFL, flags | O_NONBLOCK);

	// bind to all the local addresses and pick any port number
	memset((char*) &mMyAddr, 0, sizeof(mMyAddr));
	mMyAddr.sin_family = AF_INET;
	mMyAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	mMyAddr.sin_port = htons(0);

	if (bind(mSocket, (struct sockaddr*) &mMyAddr, sizeof(mMyAddr)) < 0) {
		mLogger->logDebug("Bind failed \n");
		cout << "Bind failed \n";
		return 0;
	}

	// define remote address, where we send the messages
	memset((char *) &mRemoteAddr, 0, mAddrLen);
	mRemoteAddr.sin_family = AF_INET;
	mRemoteAddr.sin_port = htons(mServicePort);
	if (inet_aton(mServer, &mRemoteAddr.sin_addr) == 0) {
		mLogger->logDebug("inet_aton() failed\n");
		cout << "inet_aton() failed\n";
		exit(1);
	}

	return 1;

}

void SendToHardwareViaIP::send(string msg) {
//	cout << "Send: String to send: length: "<< sizeof(msg) << ":"<<msg.length()<< " raw: "<<msg<<endl;
	char buf[(msg.length() + 1) *sizeof(char)];
	//buf[msg.length()] = 0;
	memcpy(buf, msg.c_str(), (msg.length()+1)*sizeof(char));
//	cout << "Send: buffer to send: length: "<< sizeof(buf)<<":"<<strlen(buf) << " raw: " << buf<< endl;

	// send packet
	if (sendto(mSocket, buf, (msg.length() + 1) *sizeof(char), MSG_DONTWAIT,
			(struct sockaddr *) &mRemoteAddr, mAddrLen) == -1) {
		//fprintf(stderr, "sendto");
	}
	//printf("%d\t%s\t%d\t%d\n", mSequenceNo, buf, mServicePort, mRepitition);
	mSequenceNo++;

}
