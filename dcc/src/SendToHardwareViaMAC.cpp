// This file is part of OpenC2X.
//
// OpenC2X is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenC2X is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OpenC2X.  If not, see <http://www.gnu.org/licenses/>.
//
// Authors:
// Sven Laux <slaux@mail.uni-paderborn.de>
// Gurjashan Singh Pannu <gurjashan.pannu@ccs-labs.org>
// Stefan Schneider <stefan.schneider@ccs-labs.org>
// Jan Tiemann <janhentie@web.de>


//TODO: NON BLOCKING socket?

#include "SendToHardwareViaMAC.h"
#include <ctype.h>
#include <buffers/build/data.pb.h>

using namespace std;

SendToHardwareViaMAC::SendToHardwareViaMAC(string ownerModule,string ethernetDevice, int expNo) {
	mLogger = new LoggingUtility(ownerModule, expNo);

	//has root?
	if (getuid() != 0){
		mLogger->logError("Root privileges are needed");
		exit(1);
	}

	// Sender MAC Address
	string file = string("/sys/class/net/")+ ethernetDevice + "/address";
	std::ifstream infile(file);
	getline(infile, mOwnMac);

	//check for right MAC format copied from http://stackoverflow.com/questions/4792035/how-do-you-validate-that-a-string-is-a-valid-mac-address-in-c
	int i = 0;
	int s = 0;
	const char* mac = mOwnMac.c_str();
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
		mLogger->logError("could not get a real sender Mac address. Using 12:23:34:45:56:67");
		mOwnMac = "12:23:34:45:56:67";
	}


	// Receiver MAC Address (hier: Broadcast)
	string  receiverMac = "FF:FF:FF:FF:FF:FF";
	// Name of Ethernetdevice
	string  ethDevice = ethernetDevice; //get via console: "ip link show"

	//SOCKET--------------------------------
	//create PACKET Sockets
	if ((mSocket_VI = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1)
	{
		mLogger->logPError("Socket() failed.");
		exit(1);
	}
	if ((mSocket_VO = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1)
	{
		mLogger->logPError("Socket() failed.");
		exit(1);
	}
	if ((mSocket_BE = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1)
	{
		mLogger->logPError("Socket() failed.");
		exit(1);
	}
	if ((mSocket_BK = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1)
	{
		mLogger->logPError("Socket() failed.");
		exit(1);
	}
	//set prioritys
	if (setsockopt(mSocket_VI,SOL_SOCKET ,SO_PRIORITY, &PRIORITY_VI,sizeof(PRIORITY_VI)) == -1){
		mLogger->logPError("Setsockop(priority) failed");
		exit(1);
	}
	if (setsockopt(mSocket_VO,SOL_SOCKET ,SO_PRIORITY, &PRIORITY_VO,sizeof(PRIORITY_VO)) == -1){
		mLogger->logPError("Setsockop(priority) failed");
		exit(1);
	}
	if (setsockopt(mSocket_BE,SOL_SOCKET ,SO_PRIORITY, &PRIORITY_BE,sizeof(PRIORITY_BE)) == -1){
		mLogger->logPError("Setsockop(priority) failed");
		exit(1);
	}
	if (setsockopt(mSocket_BK,SOL_SOCKET ,SO_PRIORITY, &PRIORITY_BK,sizeof(PRIORITY_BK)) == -1){
		mLogger->logPError("Setsockop(priority) failed");
		exit(1);
	}

	//get index number of Network Interface
	strncpy(mIfr.ifr_name, ethDevice.c_str(),sizeof(mIfr.ifr_name));

	if (ioctl(mSocket_VI, SIOCGIFINDEX, &mIfr) != 0){
		mLogger->logPError("ioctl (SIOCGIFINDEX) failed");
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
	memcpy(mEth_hdr.ether_shost,(u_char *)ether_aton(mOwnMac.c_str()),ETHER_ADDR_LEN);

	//ether type
	mEth_hdr.ether_type = htons(ETHERTYPE_CAR);

	//transform own Mac to fit the format of MACs incomming from the network to easier compare them
	mOwnMac = ether_ntoa(ether_aton(mOwnMac.c_str()));
}

SendToHardwareViaMAC::~SendToHardwareViaMAC() {
	close(mSocket_VO);
	close(mSocket_VI);
	close(mSocket_BE);
	close(mSocket_BK);
	delete mLogger;
}

void SendToHardwareViaMAC::send(string* msg, int priority){

	unsigned int packetsize = sizeof(struct ether_header) + msg->size();
	unsigned char packet[packetsize];
	unsigned char * payload = packet+sizeof(struct ether_header);


	//copy header to packet
	memcpy(&packet,&mEth_hdr,sizeof(struct ether_header));

	//copy payload to packet
	memcpy(payload,msg->c_str(),msg->size());

	//send Packet
	mLogger->logInfo(string("HW: sending CAR Packet on Interface ")+mIfr.ifr_name);

	int send_to_socket = -1;
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
	}

	if(send_to_socket != -1){
		/** @todo record/put into dcc info/extend to fixed size packetsize */
		if ((sendto(send_to_socket,packet,packetsize,0,(struct sockaddr* )&mTo_sock_addr,
						sizeof(struct sockaddr_ll))) == -1)
		{
				mLogger->logPError("Sendto() failed");
		}
	} else {
		mLogger->logInfo("No packet priority/queue set");
	}
}

void SendToHardwareViaMAC::sendWithGeoNet(string* msg, int priority, int type) {
	unsigned int geoHdrLen;
	uint8_t* geoHdr;
	switch(type) {
		case dataPackage::DATA_Type_DENM:
			fillGeoNetBTPheaderForDenm(msg->size());
			geoHdrLen = sizeof(struct GeoNetworkAndBTPHeaderDENM);
			geoHdr = reinterpret_cast<uint8_t*>(&mGeoBtpHdrForDenm);
			break;
		case dataPackage::DATA_Type_CAM:
			fillGeoNetBTPheaderForCam(msg->size());
			geoHdrLen = sizeof(struct GeoNetworkAndBTPHeaderCAM);
			geoHdr = reinterpret_cast<uint8_t*>(&mGeoBtpHdrForCam);
			break;
		default:
			mLogger->logError("Queued packet has invalid type: " + to_string(type));
			break;
	}
	unsigned int packetsize = sizeof(struct ether_header) + geoHdrLen + msg->size();
	unsigned char packet[packetsize];
	unsigned char* payload = packet + sizeof(struct ether_header) + geoHdrLen;
	unsigned char* geoNetHdr = packet + sizeof(struct ether_header);

	//copy header to packet
	memcpy(&packet,&mEth_hdr,sizeof(struct ether_header));

	// Fill in geo networking and btp header
	memcpy(geoNetHdr, geoHdr, geoHdrLen);

	//copy payload to packet
	memcpy(payload,msg->c_str(),msg->size());
	dumpBuffer(reinterpret_cast<const uint8_t*>(msg->c_str()), msg->size());

	//send Packet
	mLogger->logInfo(string("HW: sending CAR Packet on Interface ")+mIfr.ifr_name);

	int send_to_socket = -1;
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
	}

	if(send_to_socket != -1){
		/** @todo record/put into dcc info/extend to fixed size packetsize */
		if ((sendto(send_to_socket,packet,packetsize,0,(struct sockaddr* )&mTo_sock_addr,
						sizeof(struct sockaddr_ll))) == -1)
		{
				mLogger->logPError("Sendto() failed");
		}
	} else {
		mLogger->logInfo("No packet priority/queue set");
	}
}

void SendToHardwareViaMAC::fillGeoNetBTPheaderForCam(int payloadLen) {
	// GeoNetwork Header
	mGeoBtpHdrForCam.mGeoNetHdr.basicHeader.versionAndNH = 1;
	mGeoBtpHdrForCam.mGeoNetHdr.basicHeader.reserved = 0;
	mGeoBtpHdrForCam.mGeoNetHdr.basicHeader.lifetime = 241;
	mGeoBtpHdrForCam.mGeoNetHdr.basicHeader.remainingHopLimit = 1;

	mGeoBtpHdrForCam.mGeoNetHdr.commonHeader.nhAndReserved = 32;
	mGeoBtpHdrForCam.mGeoNetHdr.commonHeader.htAndHst = 80;
	mGeoBtpHdrForCam.mGeoNetHdr.commonHeader.tc = 2;
	mGeoBtpHdrForCam.mGeoNetHdr.commonHeader.payload = htons(payloadLen + sizeof(struct BTPHeader));
	mGeoBtpHdrForCam.mGeoNetHdr.commonHeader.maxHop = 1;
	mGeoBtpHdrForCam.mGeoNetHdr.commonHeader.reserved = 0;
	mGeoBtpHdrForCam.mGeoNetHdr.tsb.spv.addr.assignmentTypeCountryCode = htons(38393);
//	mGeoBtpHdrForCam.mGeoNetHdr.tsb.spv.addr.llAddr = reinterpret_cast<uint8_t*>(ether_aton(mOwnMac.c_str()));
	memcpy(&mGeoBtpHdrForCam.mGeoNetHdr.tsb.spv.addr.llAddr, ether_aton(mOwnMac.c_str()), ETH_ALEN);
	mGeoBtpHdrForCam.mGeoNetHdr.tsb.spv.timestamp = htonl(2810450329);
	mGeoBtpHdrForCam.mGeoNetHdr.tsb.spv.latitude = htonl(424937722);
	mGeoBtpHdrForCam.mGeoNetHdr.tsb.spv.longitude = htonl(3460636913);
	mGeoBtpHdrForCam.mGeoNetHdr.tsb.spv.speed = htons(496);
	mGeoBtpHdrForCam.mGeoNetHdr.tsb.spv.heading = htons(1996);

	mGeoBtpHdrForCam.mGeoNetHdr.tsb.reserved = htonl(0);

	// BTP Header
	mGeoBtpHdrForCam.mBTPHdr.mDestinationPort = htons(2001);
	mGeoBtpHdrForCam.mBTPHdr.mSourcePort = htons(0);
	uint8_t* temp = reinterpret_cast<uint8_t*>(&mGeoBtpHdrForCam);
	dumpBuffer(temp, sizeof(mGeoBtpHdrForCam));
}

void SendToHardwareViaMAC::fillGeoNetBTPheaderForDenm(int payloadLen) {
	// GeoNetwork Header
	mGeoBtpHdrForDenm.mGeoNetHdr.basicHeader.versionAndNH = 1;
	mGeoBtpHdrForDenm.mGeoNetHdr.basicHeader.reserved = 0;
	mGeoBtpHdrForDenm.mGeoNetHdr.basicHeader.lifetime = 241;
	mGeoBtpHdrForDenm.mGeoNetHdr.basicHeader.remainingHopLimit = 1;

	mGeoBtpHdrForDenm.mGeoNetHdr.commonHeader.nhAndReserved = 32;
	mGeoBtpHdrForDenm.mGeoNetHdr.commonHeader.htAndHst = 66;
	mGeoBtpHdrForDenm.mGeoNetHdr.commonHeader.tc = 1;
	mGeoBtpHdrForDenm.mGeoNetHdr.commonHeader.payload = htons(payloadLen + sizeof(struct BTPHeader));
	mGeoBtpHdrForDenm.mGeoNetHdr.commonHeader.maxHop = 1;
	mGeoBtpHdrForDenm.mGeoNetHdr.commonHeader.reserved = 0;

	mGeoBtpHdrForDenm.mGeoNetHdr.brdcst.sequenceNumber = 0;
	mGeoBtpHdrForDenm.mGeoNetHdr.brdcst.reserved = 0;
	mGeoBtpHdrForDenm.mGeoNetHdr.brdcst.spv.addr.assignmentTypeCountryCode = htons(38393);
//	mGeoBtpHdrForDenm.mGeoNetHdr.tsb.spv.addr.llAddr = reinterpret_cast<uint8_t*>(ether_aton(mOwnMac.c_str()));
	memcpy(&mGeoBtpHdrForDenm.mGeoNetHdr.brdcst.spv.addr.llAddr, ether_aton(mOwnMac.c_str()), ETH_ALEN);
	mGeoBtpHdrForDenm.mGeoNetHdr.brdcst.spv.timestamp = htonl(2810450329);
	mGeoBtpHdrForDenm.mGeoNetHdr.brdcst.spv.latitude = htonl(424939708);
	mGeoBtpHdrForDenm.mGeoNetHdr.brdcst.spv.longitude = htonl(-834330986);
	mGeoBtpHdrForDenm.mGeoNetHdr.brdcst.spv.speed = htons(496);
	mGeoBtpHdrForDenm.mGeoNetHdr.brdcst.spv.heading = htons(1996);

	mGeoBtpHdrForDenm.mGeoNetHdr.brdcst.latitude = htonl(424939708);
	mGeoBtpHdrForDenm.mGeoNetHdr.brdcst.longitude = htonl(-834330985);
	mGeoBtpHdrForDenm.mGeoNetHdr.brdcst.distA = 200;
	mGeoBtpHdrForDenm.mGeoNetHdr.brdcst.distB = 100;
	mGeoBtpHdrForDenm.mGeoNetHdr.brdcst.angle = 0;
	mGeoBtpHdrForDenm.mGeoNetHdr.brdcst.resrvd = htonl(0);

	// BTP Header
	mGeoBtpHdrForDenm.mBTPHdr.mDestinationPort = htons(2002);
	mGeoBtpHdrForDenm.mBTPHdr.mSourcePort = htons(0);
	uint8_t* temp = reinterpret_cast<uint8_t*>(&mGeoBtpHdrForDenm);
	dumpBuffer(temp, sizeof(mGeoBtpHdrForDenm));
}

void SendToHardwareViaMAC::dumpBuffer(const uint8_t* buffer, int size) {
	for (int i = 0; i < size; i++) {
		if (i > 0) printf(":");
		printf("%02X", buffer[i]);
	}
	printf("\n");
}
