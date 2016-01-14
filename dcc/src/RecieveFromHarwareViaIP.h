/*
 * RecieveFromHarwareViaIP.h
 *
 *  Created on: Jan 14, 2016
 *      Author: jan
 */

#ifndef RECIEVEFROMHARWAREVIAIP_H_
#define RECIEVEFROMHARWAREVIAIP_H_

#include <utility/CommunicationSender.h>
#include <sys/socket.h>
#include <netdb.h>
#include <boost/thread.hpp>
#include <zmq.hpp>

#define BUFSIZE 4096

class RecieveFromHarwareViaIP {
public:
	RecieveFromHarwareViaIP();
	virtual ~RecieveFromHarwareViaIP();
	void init();
	void recieve();

private:
	struct sockaddr_in mMyaddr;	/* our address */
	struct sockaddr_in mRemoteAddr;	/* remote address */
	socklen_t mAddrlen = sizeof(mRemoteAddr);		/* length of addresses */
	int mRecvlen = 0;			/* # bytes received */
	int mSocket =0;					/* our socket */
	unsigned char mRecvBuffer[BUFSIZE];	/* receive buffer */
	int mService_port = 21234;
	int mRepetition = 0;

	CommunicationSender* mSenderToDcc;
	boost::thread* mThreadReceive;
	LoggingUtility* mLogger;
};

#endif /* RECIEVEFROMHARWAREVIAIP_H_ */
