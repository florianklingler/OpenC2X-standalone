#ifndef SENDTOHARDWAREVIAMAC_H_
#define SENDTOHARDWAREVIAMAC_H_

/**
 * @addtogroup dccHardware
 * @{
 */

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

};

/**
 * @}
 */

#endif /* SENDTOHARDWAREVIAMAC_H_ */
