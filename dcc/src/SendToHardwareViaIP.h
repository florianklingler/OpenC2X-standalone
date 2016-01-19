#ifndef SENDTOHARDWAREVIAIP_H_
#define SENDTOHARDWAREVIAIP_H_

#include <utility/LoggingUtility.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

#define BUFSIZE 4096

class SendToHardwareViaIP {
public:
	SendToHardwareViaIP();
	virtual ~SendToHardwareViaIP();
	bool init();
	void send(string msg);

private:
	LoggingUtility* mLogger;

	const char* mServer;
	int mServicePort = 21234;
	int mSocket;
	struct sockaddr_in mMyAddr;
	struct sockaddr_in mRemoteAddr;
	socklen_t mAddrLen = sizeof(mRemoteAddr);
	int mRepitition = 0;
	int mSequenceNo = 0;
};

#endif /* SENDTOHARDWAREVIAIP_H_ */
