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
	//TODO: rename to fit to convention
	struct sockaddr_in myaddr;	/* our address */
	struct sockaddr_in remaddr;	/* remote address */
	socklen_t addrlen = sizeof(remaddr);		/* length of addresses */
	int recvlen = 0;			/* # bytes received */
	int fd =0;					/* our socket */
	unsigned char buf[BUFSIZE];	/* receive buffer */
	int service_port = 21234;
	int repetition = 0;

	CommunicationSender* mSenderToDcc;
	boost::thread* mThreadReceive;
	LoggingUtility* mLogger;
};

#endif /* RECIEVEFROMHARWAREVIAIP_H_ */
