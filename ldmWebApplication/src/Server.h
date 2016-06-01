/*
 * Server.h
 *
 *  Created on: May 25, 2016
 *      Author: root
 */

#ifndef SERVER_H_
#define SERVER_H_

#include <utility/CommunicationClient.h>

class Server {
public:
	Server();
	virtual ~Server();
	//void run();
	void request();

private:
	CommunicationClient* mClientLdm;
};

#endif /* SERVER_H_ */
