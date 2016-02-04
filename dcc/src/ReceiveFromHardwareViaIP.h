#ifndef RECEIVEFROMHARDWAREVIAIP_H_
#define RECEIVEFROMHARDWAREVIAIP_H_

#include <utility/CommunicationSender.h>
#include <sys/socket.h>
#include <netdb.h>
#include <boost/thread.hpp>
#include <zmq.hpp>

#define BUFSIZE 2048

class DCC;

class ReceiveFromHardwareViaIP {
public:
	ReceiveFromHardwareViaIP(DCC* dcc);
	virtual ~ReceiveFromHardwareViaIP();
	void init();
	void receive();

private:
	struct sockaddr_in mMyAddr;	/* our address */
	struct sockaddr_in mRemoteAddr;	/* remote address */
	socklen_t mAddrLen = sizeof(mRemoteAddr);		/* length of addresses */
	int mRecvLen = 0;			/* # bytes received */
	int mSocket =0;					/* our socket */
	unsigned char mRecvBuffer[BUFSIZE];	/* receive buffer */
	int mServicePort = 21234;
	int mRepetition = 0;

	DCC* mOwner;
	boost::thread* mThreadReceive;
	LoggingUtility* mLogger;
};

#endif /* RECEIVEFROMHARDWAREVIAIP_H_ */
