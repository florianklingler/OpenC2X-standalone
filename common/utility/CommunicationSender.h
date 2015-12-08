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
	CommunicationSender(string portOut, string envelope);
	~CommunicationSender();
	virtual void send(string msg);



protected:
	zmq::context_t* mContext;
	zmq::socket_t* mPublisher;

	string mEnvelope;
};

#endif /* UTILITY_COMMUNICATIONSENDER_H_ */
