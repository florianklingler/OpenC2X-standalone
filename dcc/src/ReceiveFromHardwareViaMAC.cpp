/*
 * ReceiveFromHardwareViaMAC.cpp
 *
 *  Created on: Apr 20, 2016
 *      Author: root
 */

#include "ReceiveFromHardwareViaMAC.h"

ReceiveFromHardwareViaMAC::ReceiveFromHardwareViaMAC(string ownerModule) {
	mLogger = new LoggingUtility(ownerModule);

	//has root?
	if (getuid() != 0){
		mLogger->logDebug("Program needs root privileges");
		cout << "Program needs root privileges";
		exit(1);
	}

	//PACKET SOCKET, receives ALL incoming packages in whole (with all headers)
	mSocket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (mSocket == -1){
		mLogger->logDebug("socket creation failed");
		perror("socket() failed");
		exit(1);
	}

}

ReceiveFromHardwareViaMAC::~ReceiveFromHardwareViaMAC() {
	delete mLogger;
	close(mSocket);
}

/**
 * receives packages till a CAR Communication package is found and returns its payload
 *
 * @return <MAC address of sender,payload>
 */
pair<string,string> ReceiveFromHardwareViaMAC::receive(){

	while(1){
		//receive package, blocking
		mBytes = read(mSocket, mPacket, sizeof(mPacket));
		if (mBytes == -1){
			mLogger->logDebug("reading from Socket failed");
			perror("read() failed");
			exit(1);
		}

		if(mEth_hdr->ether_type != ETHERTYPE_CAR){
			//not Car communication package, ignore! restart while loop and read next package
			continue;
		}

		int payloadLength = mBytes - mLinkLayerLength;
		//convert sender Mac from network byte order to char
		string senderMac = ether_ntoa((struct ether_addr* )mEth_hdr->ether_shost);
		string msg = string(mPayload,payloadLength);

		return make_pair(senderMac,msg);
	}
}






