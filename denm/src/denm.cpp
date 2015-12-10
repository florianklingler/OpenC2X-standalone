#include "denm.h"
#include <buffers/build/denm.pb.h>
#include <google/protobuf/text_format.h>
#include <unistd.h>
#include <string>
#include <iostream>

using namespace std;

DENM::DENM () {
	mCommunicationDccToLdm = new Communication("5555", "9999", "DENM", this);
	mSenderDcc = new CommunicationSender("7777");
}

DENM::~DENM() {
	mToDccThread->join();
	mDccToLdmThread->join();
}

void DENM::init() {
	mToDccThread = new boost::thread(&DENM::sendTestLoop, this);
	mDccToLdmThread = new boost::thread(&Communication::run, mCommunicationDccToLdm);
}

string DENM::process(string message) {
	cout << "forward incoming DENM to LDM" << endl;
	return message;
}


void DENM::sendTestLoop(){
	//cout << "started send loop" << endl;
	denmPackage::DENM msgDenmSend;
	string msg;
	msgDenmSend.set_id(12);
	msgDenmSend.set_content("DENM from DENM service");
	msgDenmSend.SerializeToString(&msg);
	while(1) {
		sleep(1);
		cout << "send new DENM to LDM" << endl;
		mCommunicationDccToLdm->send("DENM", msg);
		cout << "send new DENM to DCC" << endl;
		mSenderDcc->send("DENM", msg);
	}
}

int main () {
  	DENM denm;
  	denm.init();

  	return EXIT_SUCCESS;
}
