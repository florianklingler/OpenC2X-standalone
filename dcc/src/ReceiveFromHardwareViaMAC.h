#ifndef RECEIVEFROMHARDWAREVIAMAC_H_
#define RECEIVEFROMHARDWAREVIAMAC_H_

#include <utility/Constants.h>
#include <utility/LoggingUtility.h>
#include <string>
#include <unistd.h> // Std. Fct.  getuid() and read()
#include <netinet/in.h> // Socket Fct. (incl. <sys/socket.h>)
#include <netinet/ether.h> // Ethernet Fct. u. Kcnst.
// (ETH_P_ALL or ether_ntoa())
// (struct in_addr or inet_ntoa())
#include <net/ethernet.h> // Ethernet Header Structure

class ReceiveFromHardwareViaMAC {
public:
	ReceiveFromHardwareViaMAC(std::string ownerModule, int expNo);
	virtual ~ReceiveFromHardwareViaMAC();
	void init();
	std::pair<std::string,std::string> receive();

private:
	LoggingUtility* mLogger;

	int mSocket;
    char mPacket[ETHERMTU];

    //num of bytes received
    int mBytes = 0;

    // pointers to access header and payload
    int mLinkLayerLength = sizeof(struct ether_header);
    struct ether_header* mEth_hdr = (struct ether_header*) mPacket;;
    char* mPayload = mPacket + mLinkLayerLength;
};

#endif /* RECEIVEFROMHARDWAREVIAMAC_H_ */
