#include "Communication.h"

Communication::Communication(string portIn, string portOut, string envelope, ICommunication* communicator):
CommunicationSender(portOut, envelope){
	mCommunicator = communicator;

  	//subscriber for receiving
  	mSubscriber = new zmq::socket_t(*mContext, ZMQ_SUB);

  	mSubscriber->connect("tcp://localhost:"+portIn);
  	mSubscriber->setsockopt(ZMQ_SUBSCRIBE, envelope.c_str(), 1);

}

string Communication::receive(){
	string envelope = s_recv(*mSubscriber);
	string msg = s_recv(*mSubscriber);
	return msg;
}

void Communication::run(){
	while(true){
		string message = receive();
		string newMessage = mCommunicator->process(message);
		send(newMessage);
	}
}
