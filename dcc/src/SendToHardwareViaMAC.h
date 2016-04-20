#ifndef SENDTOHARDWAREVIAMAC_H_
#define SENDTOHARDWAREVIAMAC_H_

#include <utility/LoggingUtility.h>
#include <string.h>



static uint16_t ETHERTYPE_CAR = 0x0CA4;

class SendToHardwareViaMAC {
public:
	SendToHardwareViaMAC();
	virtual ~SendToHardwareViaMAC();
	bool init();
	void send(string* msg, int priority);
private:
	LoggingUtility* mLogger;

	int mSocket;
	struct ether_header mEth_hdr;
	//information of network interface
	struct ifreq mIfr;
	//socket address data structure for PACKET sockets
	struct sockaddr_ll mTo_sock_addr;

};

#endif /* SENDTOHARDWAREVIAMAC_H_ */
