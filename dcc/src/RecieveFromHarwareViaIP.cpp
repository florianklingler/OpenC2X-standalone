#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE


#include "RecieveFromHarwareViaIP.h"
#include <string>
#include <arpa/inet.h>
#include <sstream>


RecieveFromHarwareViaIP::RecieveFromHarwareViaIP() {

	mSenderToDcc = new CommunicationSender("RecieveFromHardware", "4444");
	mLogger = new LoggingUtility("RecieveFromHardware");
}

RecieveFromHarwareViaIP::~RecieveFromHarwareViaIP() {
	mThreadReceive->join();
}

void RecieveFromHarwareViaIP::init(){
	/* create a UDP socket */

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		mLogger->logDebug("cannot create socket\n");
		return;
	}


	/* bind the socket to any valid IP address and a specific port */

	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(service_port);

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		mLogger->logDebug("bind failed");
		return;
	}



	mThreadReceive = new boost::thread(&RecieveFromHarwareViaIP::recieve, this);


}

void RecieveFromHarwareViaIP::recieve(){
	int seqno = 0;

	mLogger->logDebug("seqno\tdata\tport\trepetition\n");
	fflush(stdout);
	/* now loop, receiving data and printing what we received */
		while(true) {
			//fprintf(stderr, "waiting on port %d\n", service_port);
			recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
			//printf("received %d bytes\n", recvlen);
			if (recvlen > 0) {
				buf[recvlen] = 0;
				//printf("received message: \"%s\"\n", buf);

				std::ostringstream oss;
				oss <<  "\n seqno: " << seqno << " buf: "<< buf
						<< " serservice_port: " << service_port
						<< " repetition: " << repetition << std::endl;
				mLogger->logDebug(oss.str());
				//send to dcc
				std::string tmp(reinterpret_cast<char*>(buf));
				mSenderToDcc->sendToHw(tmp);
			}
			seqno++;
		}
		/* never exits */
}
