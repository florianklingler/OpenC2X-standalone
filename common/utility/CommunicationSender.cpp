/*
 * CommunicationSender.cpp
 *
 *  Created on: 08.12.2015
 *      Author: sven
 */

#include <utility/CommunicationSender.h>
#include <google/protobuf/text_format.h>

CommunicationSender::CommunicationSender(string portOut, string envelope) {
	mEnvelope = envelope;

	GOOGLE_PROTOBUF_VERIFY_VERSION;

	mContext = new zmq::context_t(1);
	mPublisher = new zmq::socket_t(*mContext, ZMQ_PUB);
	mPublisher->bind("tcp://*:"+portOut);
}

CommunicationSender::~CommunicationSender() {
	// TODO Auto-generated destructor stub
}


void CommunicationSender::send(string msg){
	s_sendmore(*mPublisher, mEnvelope);
	s_send(*mPublisher, msg);
}
