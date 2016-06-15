#include "ReceiveFromHardwareViaMAC.h"

using namespace std;

ReceiveFromHardwareViaMAC::ReceiveFromHardwareViaMAC(string ownerModule, int expNo) {
	mLogger = new LoggingUtility(ownerModule, expNo);

	//has root?
	if (getuid() != 0){
		mLogger->logInfo("Program needs root privileges");
		exit(1);
	}

	//PACKET SOCKET, receives ALL incoming packages in whole (with all headers)
	mSocket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (mSocket == -1){
		mLogger->logPError("socket creation failed");
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
			mLogger->logPError("reading from Socket failed");
			exit(1);
		}

		if(ntohs(mEth_hdr->ether_type) != ETHERTYPE_CAR){ // TODO: optimization: put this check into hw by creating the socets with ethertype_car instead of ALL ??
			//not Car communication package, ignore! restart while loop and read next package
			continue;
		}

		 /** not needed right now
		 * get time of package arrival
			time_t curtime;
			struct timeval tv;
			int milliseconds;
			char time_buffer[30];

			ioctl(s, SIOCGSTAMP, &tv);
			curtime=tv.tv_sec;
			milliseconds=(int)tv.tv_usec;
			strftime (time_buffer, 30, "%Y-%m-%d, %T", localtime(&curtime));
			printf("nTime: %s.%06d\n", time_buffer, milliseconds);

		 */

		int payloadLength = mBytes - mLinkLayerLength;
		//convert sender Mac from network byte order to char
		string senderMac = ether_ntoa((struct ether_addr* )mEth_hdr->ether_shost);
		string msg = string(mPayload,payloadLength);

		return make_pair(senderMac,msg);
	}
}
