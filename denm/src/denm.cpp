#include "denm.h"
#include <utility/zhelpers.hpp>
#include <buffers/build/denm.pb.h>
#include <zmq.hpp>
#include <unistd.h>
#include <string>
#include <iostream>
#include <google/protobuf/text_format.h>

using namespace std;
using namespace zmq;

DENM::DENM () {
	mCommunicationDccToLdm = new Communication("5555", "9999", "DENM", this);
	mSenderDcc = new CommunicationSender("7777", "DENM");
}

DENM::~DENM() {
	mSendToDccThread->join();
	mReceiveFromDccThread->join();
}

void DENM::init() {
	mSendToDccThread = new boost::thread(&DENM::sendTestLoop, this);
	mReceiveFromDccThread = new boost::thread(&Communication::run, mCommunicationDccToLdm);
}

string DENM::process(string message) {
	return message;
}


void DENM::sendTestLoop(){
	denmPackage::DENM msgDenmSend;
	string msg;
	msgDenmSend.set_id(12);
	msgDenmSend.set_content("DENM from DENM service");
	msgDenmSend.SerializeToString(&msg);
	while(1) {
		sleep(1);
		mCommunicationDccToLdm->send(msg);
		mSenderDcc->send(msg);
	}
}

int main () {
  	DENM denm;
  	denm.init();

  	return EXIT_SUCCESS;
}
