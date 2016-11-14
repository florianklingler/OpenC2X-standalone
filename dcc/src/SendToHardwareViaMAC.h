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


#ifndef SENDTOHARDWAREVIAMAC_H_
#define SENDTOHARDWAREVIAMAC_H_

/**
 * @addtogroup dccHardware
 * @{
 */

#include "GeoNetHeaders.h"

#include <utility/LoggingUtility.h>
#include <utility/Constants.h>
#include <string.h>

#include <unistd.h> // Std. Fct.  getuid() and read()
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/ether.h>

#include <linux/if_packet.h>

#include <sys/ioctl.h>
#include <net/if.h>

/**
 * Sends messages to Hardware to be broadcasted on the MAC layer. Uses RAW_SOCKET so root is needed.
 * Data is wrapped in a minimal Ethernet packet containing only a Ethernet header and the payload.
 * No IP header.
 * All data is broadcasted on the MAC layer.
 *
 *
 * @nonStandard serialization is not standard conform. Uses protobuffer instead.
 */
class SendToHardwareViaMAC {
public:
	/**
	 * ownerModule and expNo forwarded to LoggingUtility constructor
	 * @param ownerModule Module Name
	 * @param expNo Experiment Number
	 * @param ethernetDevice Device used for sending
	 */
	SendToHardwareViaMAC(std::string ownerModule,std::string ethernetDevice, int expNo);
	virtual ~SendToHardwareViaMAC();

	/**
	 * Sends msg to the hardware queue with the corresponding priority.
	 * @param msg
	 * @param priority
	 */
	void send(std::string* msg, int priority);

	/**
	 * Sends standard compliant packet to the hardware queue with the corresponding priority.
	 * @param msg
	 * @param priority
	 * @param type
	 */
	void sendWithGeoNet(std::string* msg, int priority, int type);

	/**
	 * Fills simple GeoNetworking and BTP header for CAM
	 * @returns geonetwork and BTP header
	 */
	void fillGeoNetBTPheaderForCam(int payloadLen);

	void fillGeoNetBTPheaderForDenm(int payloadLen);

	/**
	 * Dumps the content in the buffer in hex format
	 */
	void dumpBuffer(const uint8_t* buffer, int size);

	std::string mOwnMac;
private:
	LoggingUtility* mLogger;

	int mSocket_VI;
	int mSocket_VO;
	int mSocket_BE;
	int mSocket_BK;
	//Ethernet header
	struct ether_header mEth_hdr;
	//information of network interface
	struct ifreq mIfr;
	//socket address data structure for PACKET sockets
	struct sockaddr_ll mTo_sock_addr;

	// Basic struct for geo-networking and BTP header
	// Hard-coded for interoperability with Cohda MK5 box
	struct GeoNetworkAndBTPHeaderCAM mGeoBtpHdrForCam;
	struct GeoNetworkAndBTPHeaderDENM mGeoBtpHdrForDenm;
};

/**
 * @}
 */

#endif /* SENDTOHARDWAREVIAMAC_H_ */
