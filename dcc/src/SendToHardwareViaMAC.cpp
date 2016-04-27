//TODO: NON BLOCKING socket?

#include "SendToHardwareViaMAC.h"
#include <ctype.h>


SendToHardwareViaMAC::SendToHardwareViaMAC(string ethernetDevice) {
	mLogger = new LoggingUtility("SendToHardware");

	//has root?
	if (getuid() != 0){
		mLogger->logDebug("Program needs root privileges");
		cout << "Program needs root privileges";
		exit(1);
	}


	// Sender MAC Address
	string file = string("/sys/class/net/")+ ethernetDevice + "/address";
	std::ifstream infile(file);
	string senderMac;
	getline(infile, senderMac);

	//check for right MAC format copied from http://stackoverflow.com/questions/4792035/how-do-you-validate-that-a-string-is-a-valid-mac-address-in-c
	int i = 0;
	int s = 0;
	const char* mac = senderMac.c_str();
	while (*mac) {
	   if (isxdigit(*mac)) {
		  i++;
	   }
	   else if (*mac == ':' || *mac == '-') {

		  if (i == 0 || i / 2 - 1 != s)
			break;

		  ++s;
	   }
	   else {
		   s = -1;
	   }


	   ++mac;
	}

	if (!(i == 12 && (s == 5 || s == 0))){
		mLogger->logDebug("could not get a real sender Mac address. Using 12::23:34:45:56:67");
		senderMac = "12::23:34:45:56:67";
	}


	// Receiver MAC Address (hier: Broadcast)
	string  receiverMac = "FF:FF:FF:FF:FF:FF";
	// Name of Ethernetdevice
	string  ethDevice = ethernetDevice; //get via console: "ip link show"

	//SOCKET--------------------------------
	//create PACKET Sockets
	if ((mSocket_VI = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1)
	{
		mLogger->logDebug("Socket() failed.");
		perror("Socket() failed");
		exit(1);
	}
	if ((mSocket_VO = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1)
	{
		mLogger->logDebug("Socket() failed.");
		perror("Socket() failed");
		exit(1);
	}
	if ((mSocket_BE = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1)
	{
		mLogger->logDebug("Socket() failed.");
		perror("Socket() failed");
		exit(1);
	}
	if ((mSocket_BK = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1)
	{
		mLogger->logDebug("Socket() failed.");
		perror("Socket() failed");
		exit(1);
	}
	//set prioritys
	if (setsockopt(mSocket_VI,SOL_SOCKET ,SO_PRIORITY, &PRIORITY_VI,sizeof(PRIORITY_VI)) == -1){
		mLogger->logDebug("Setsockop(priority) failed");
		perror("setsockopt() failed");
		exit(1);
	}
	if (setsockopt(mSocket_VO,SOL_SOCKET ,SO_PRIORITY, &PRIORITY_VO,sizeof(PRIORITY_VO)) == -1){
		mLogger->logDebug("Setsockop(priority) failed");
		perror("setsockopt() failed");
		exit(1);
	}
	if (setsockopt(mSocket_BE,SOL_SOCKET ,SO_PRIORITY, &PRIORITY_BE,sizeof(PRIORITY_BE)) == -1){
		mLogger->logDebug("Setsockop(priority) failed");
		perror("setsockopt() failed");
		exit(1);
	}
	if (setsockopt(mSocket_BK,SOL_SOCKET ,SO_PRIORITY, &PRIORITY_BK,sizeof(PRIORITY_BK)) == -1){
		mLogger->logDebug("Setsockop(priority) failed");
		perror("setsockopt() failed");
		exit(1);
	}

	//get index number of Network Interface
	strncpy(mIfr.ifr_name, ethDevice.c_str(),sizeof(mIfr.ifr_name));

	if (ioctl(mSocket_VI, SIOCGIFINDEX, &mIfr) != 0){
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
	close(mSocket_VO);
	close(mSocket_VI);
	close(mSocket_BE);
	close(mSocket_BK);
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

	int send_to_socket;
	switch(priority){
		case PRIORITY_VI:
			send_to_socket = mSocket_VI;
			break;
		case PRIORITY_VO:
			send_to_socket = mSocket_VO;
			break;
		case PRIORITY_BE:
			send_to_socket = mSocket_BE;
			break;
		case PRIORITY_BK:
			send_to_socket = mSocket_BK;
			break;
		default:
                        send_to_socket = -1;
			mLogger->logDebug("No packet priority/queue set");
	}

	if(send_to_socket != -1){
            if ((sendto(send_to_socket,packet,packetsize,0,(struct sockaddr* )&mTo_sock_addr,
                            sizeof(struct sockaddr_ll))) == -1)
            {
                    mLogger->logDebug("Sendto() failes");
                    perror("Sendto() failed");
            }
        }
}







