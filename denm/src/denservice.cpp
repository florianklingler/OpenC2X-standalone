#include "denservice.h"

#include <buffers/build/denm.pb.h>
#include <google/protobuf/text_format.h>
#include <unistd.h>
#include <iostream>
#include <string>

using namespace std;

DenService::DenService () {
	mReceiverFromDcc = new CommunicationReceiver("5555", "DENM");
	mSenderToDcc = new CommunicationSender("7777");
	mSenderToLdm = new CommunicationSender("9999");
}

DenService::~DenService() {
	mThreadReceive->join();
	mThreadSend->join();
}

void DenService::init() {
	mThreadReceive = new boost::thread(&DenService::receive, this);
	mThreadSend = new boost::thread(&DenService::send, this);
}

void DenService::receive() {
	string envelope;		//envelope
	string byteMessage;		//byte string (serialized DENM)
	while(1) {
		pair<string, string> received = mReceiverFromDcc->receive();
		envelope = received.first;
		byteMessage = received.second;

		cout << "forward incoming DENM to LDM" << endl;
		mSenderToLdm->send(envelope, byteMessage);
	}
}

void DenService::send(){
	denmPackage::DENM message;
	string byteMessage;
	message.set_id(12);
	message.set_content("DENM from DENM service");
	message.SerializeToString(&byteMessage);
	while(1) {
		sleep(1);
		cout << "send new DENM to LDM and DCC" << endl;
		mSenderToLdm->send("DENM", byteMessage);
		mSenderToDcc->send("DENM", byteMessage);
	}
}

int main () {
  	DenService denm;
  	denm.init();

  	return EXIT_SUCCESS;
}
