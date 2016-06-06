#ifndef SERVER_H_
#define SERVER_H_

#include <utility/CommunicationClient.h>
#include <utility/LoggingUtility.h>

class Server {
public:
	Server();
	virtual ~Server();
	//void run();
	void requestData();

private:
	CommunicationClient* mClientLdm;

	LoggingUtility* mLogger;
};

#endif /* SERVER_H_ */
