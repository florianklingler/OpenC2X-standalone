#include "Communication.h"

Communication::Communication(string portIn, string portOut, string envelope, ICommunication* communicator):
CommunicationSender(portOut), CommunicationReceiver(portIn, envelope){
	mCommunicator = communicator;
}

void Communication::run(){
	while(true){
		string envelope = receive().first;
		string message = receive().second;
		string newMessage = mCommunicator->process(message);
		send(envelope, newMessage);
	}
}
