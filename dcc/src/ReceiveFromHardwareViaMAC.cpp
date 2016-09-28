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


#include "ReceiveFromHardwareViaMAC.h"
#include "GeoNetHeaders.h"

using namespace std;

ReceiveFromHardwareViaMAC::ReceiveFromHardwareViaMAC(string ownerModule, int expNo) {
	mLogger = new LoggingUtility(ownerModule, expNo);

	//has root?
	if (getuid() != 0){
		mLogger->logInfo("Program needs root privileges");
		exit(1);
	}

	// PACKET SOCKET, receives ALL incoming packages in whole (with all headers)
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

pair<string,string> ReceiveFromHardwareViaMAC::receive() {

	while(1){
		//receive package, blocking
		mBytes = read(mSocket, mPacket, sizeof(mPacket));
		if (mBytes == -1){
			mLogger->logPError("reading from Socket failed");
			exit(1);
		}

		if(ntohs(mEth_hdr->ether_type) != ETHERTYPE_CAR) { // TODO: optimization: put this check into hw by creating the socets with ethertype_car instead of ALL ??
			//not Car communication package, ignore! restart while loop and read next package
			continue;
		}

		 /**
		  * not needed right now
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
		string senderMac = ether_ntoa((struct ether_addr*)mEth_hdr->ether_shost);
		string msg = string(mPayload,payloadLength);

		return make_pair(senderMac,msg);
	}
}

pair<ReceivedPacketInfo, string> ReceiveFromHardwareViaMAC::receiveWithGeoNetHeader() {
	while(1) {
		// receive package, blocking
		mBytes = read(mSocket, mPacket, sizeof(mPacket));
		if (mBytes == -1) {
			mLogger->logPError("reading from Socket failed");
			exit(1);
		}
		if (ntohs(mEth_hdr->ether_type) != ETHERTYPE_CAR) { // TODO: optimization
			// not GeoNetworking ethertype, ignore! Read next package
			continue;
		}
		mLogger->logInfo("GeoNetworking PDU");
		int geoNetPDULen = mBytes - mLinkLayerLength;
		// convert sender Mac from network byte order to char
		string senderMac = ether_ntoa((struct ether_addr*)mEth_hdr->ether_shost);
		// Hack! As of now, we are looking for very specific bits in the GeoNetworking header
		char* geoNetPDU = mPacket + mLinkLayerLength;
		if (geoNetPDU[5] == 80) {
			// CAM
			int camPDULen = geoNetPDULen - sizeof(struct GeoNetworkAndBTPHeader);
			char* camPDU = geoNetPDU + sizeof(struct GeoNetworkAndBTPHeader);
			string msg(camPDU, camPDULen);
			mLogger->logInfo("Received CAM of size: " + to_string(camPDULen) + ", forwarding it to the CAM service");
			ReceivedPacketInfo info;
			info.mSenderMac = senderMac;
			info.mType = dataPackage::DATA_Type_CAM;
			return make_pair(info, msg);
		} else if (geoNetPDU[5] == 66) {
			// DENM
			mLogger->logInfo("Received DENM but we do not handle standard compliant DENMs at the moment.");
			continue;
		} else {
			// TODO: Possible cases?
			continue;
		}
	}
}
