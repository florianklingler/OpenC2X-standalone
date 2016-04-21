/*
 * ReceiveFromHardwareViaMAC.h
 *
 *  Created on: Apr 20, 2016
 *      Author: root
 */

#ifndef RECEIVEFROMHARDWAREVIAMAC_H_
#define RECEIVEFROMHARDWAREVIAMAC_H_


#include <utility/LoggingUtility.h>
#include <string>
#include <unistd.h> // Std. Fct.  getuid() and read()
#include <netinet/in.h> // Socket Fct. (incl. <sys/socket.h>)
#include <netinet/ether.h> // Ethernet Fct. u. Kcnst.
// (ETH_P_ALL or ether_ntoa())
// (struct in_addr or inet_ntoa())
#include <net/ethernet.h> // Ethernet Header Structure

static uint16_t ETHERTYPE_CAR = 0x0CA4;

using namespace std;

class ReceiveFromHardwareViaMAC {
public:
	ReceiveFromHardwareViaMAC(string ownerModule);
	virtual ~ReceiveFromHardwareViaMAC();
	void init();
	pair<string,string> receive();

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

};

#endif /* RECEIVEFROMHARDWAREVIAMAC_H_ */
