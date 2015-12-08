#include "Communication.h"
#include <utility/zhelpers.hpp>


Communication::Communication (int portIn, string envelope, int portOut, string (*process)(string message)) {
	this.mProcess = process;
	this.mPortOut = portOut;
	this.mEnvelope = envelope;

	GOOGLE_PROTOBUF_VERIFY_VERSION;

	mContext = new zmq::context_t(1);

  	//subscriber for receiving
  	mSubscriber = new zmq::socket_t(*mContext, ZMQ_SUB);
  	mSubscriber->connect ("tcp://localhost:"+portIn);
  	mSubscriber->setsockopt ( ZMQ_SUBSCRIBE, envelope, 1);

  	//publisher for sending
	mPublisher = new zmq::socket_t(*mContext, ZMQ_PUB);
	mPublisher->bind("tcp://*:"+portOut);
}


string Communication::receive(){
	string envelope = s_recv(*mSubscriber);
	string msg = s_recv(*mSubscriber);
	return msg;
}


void Communication::send(String msg){
	//Send
	s_sendmore(*mPublisher, mEnvelope);
	s_send(*mPublisher, msg);
}

void Communication::run(){
	while(true){
		string message = receive();
		string newMessage = mProcess(message);
		send(newMessage);
	}
}
