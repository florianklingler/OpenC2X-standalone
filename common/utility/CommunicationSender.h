/*
 * CommunicationSender.h
 *
 *  Created on: 08.12.2015
 *      Author: sven
 */

#ifndef UTILITY_COMMUNICATIONSENDER_H_
#define UTILITY_COMMUNICATIONSENDER_H_

#include <zmq.hpp>
#include <string>
#include <utility/zhelpers.hpp>

using namespace std;

class CommunicationSender {
public:
	CommunicationSender(string portOut);
	~CommunicationSender();
	virtual void send(string envelope, string message);



protected:
	zmq::context_t* mContext;
	zmq::socket_t* mPublisher;
};

#endif /* UTILITY_COMMUNICATIONSENDER_H_ */
