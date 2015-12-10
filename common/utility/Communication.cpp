#include "Communication.h"

Communication::Communication(string portIn, string portOut, string envelope, ICommunication* communicator):
CommunicationSender(portOut), CommunicationReceiver(portIn, envelope){
	mCommunicator = communicator;
}

void Communication::run() {
	while(true){
		pair<string, string> received = receive();
		string envelope = received.first;
		string message = received.second;
		string newMessage = mCommunicator->process(message);
		send(envelope, message);
	}
}
