//TODO: NON BLOCKING socket?

#include "SendToHardwareViaMAC.h"



SendToHardwareViaMAC::SendToHardwareViaMAC(string ethernetDevice) {
	mLogger = new LoggingUtility("SendToHardware");

	//has root?
	if (getuid() != 0){
		mLogger->logDebug("Program needs root privileges");
		cout << "Program needs root privileges";
		exit(1);
	}


	// Sender MAC Address
	string senderMac = "12:23:34:45:56:67"; //TODO: get real mac
	// Receiver MAC Address (hier: Broadcast)
	string  receiverMac = "FF:FF:FF:FF:FF:FF";
	// Name of Ethernetdevice
	string  ethDevice = ethernetDevice; //get via console: "ip link show"

	//SOCKET--------------------------------
	//create PACKET Socket
	if ((mSocket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1)
	{
		mLogger->logDebug("Socket() failed.");
		perror("Socket() failed");
		exit(1);
	}
	//set priority
	int socket_priority = 1;
	if (setsockopt(mSocket,SOL_SOCKET ,SO_PRIORITY, &socket_priority,sizeof(socket_priority)) == -1){
		mLogger->logDebug("Setsockop(priority) failed");
		perror("setsockopt() failed");
		exit(1);
	}

	//get index number of Network Interface
	strncpy(mIfr.ifr_name, ethDevice.c_str(),sizeof(mIfr.ifr_name));

	if (ioctl(mSocket, SIOCGIFINDEX, &mIfr) != 0){
		mLogger->logDebug("ioctl (SIOCGIFINDEX) failed");
		perror("ioctl (SIOCGIFINDEX) failed");
		exit(1);
	}

	//define socket address
	memset(&mTo_sock_addr, 0, sizeof(struct sockaddr_ll));
	mTo_sock_addr.sll_ifindex = mIfr.ifr_ifindex;

	//ETHERNET HEADER-------------------------

	//fill ethernet header

	//Destination mac
	memcpy(mEth_hdr.ether_dhost,(u_char *)ether_aton(receiverMac.c_str()),ETHER_ADDR_LEN);

	//Source Mac
	memcpy(mEth_hdr.ether_shost,(u_char *)ether_aton(senderMac.c_str()),ETHER_ADDR_LEN);

	//ether type
	mEth_hdr.ether_type = htons(ETHERTYPE_CAR);


}

SendToHardwareViaMAC::~SendToHardwareViaMAC() {
	close(mSocket);
	delete mLogger;
}

void SendToHardwareViaMAC::send(string* msg, int priority){
	//TODO: currently priority is set in the init function and this value is ignored

	unsigned int packetsize = sizeof(struct ether_header) + msg->size();
	unsigned char packet[packetsize];
	unsigned char * payload = packet+sizeof(struct ether_header);


	//copy header to packet
	memcpy(&packet,&mEth_hdr,sizeof(struct ether_header));

	//copy payload to packet
	memcpy(payload,msg->c_str(),msg->size());

	//send Packet
	printf("sending CAR Packet on Interface %s (%i)...\n",
			mIfr.ifr_name, mIfr.ifr_ifindex);
	if ((sendto(mSocket,packet,packetsize,0,(struct sockaddr* )&mTo_sock_addr,
			sizeof(struct sockaddr_ll))) == -1)
	{
		perror("Sendto() failed");
		exit(1);
	}

}







