#include "denservice.h"

#include <google/protobuf/text_format.h>
#include <unistd.h>
#include <iostream>
#include <ctime>
#include <chrono>
#include <string>

INITIALIZE_EASYLOGGINGPP

using namespace std;

DenService::DenService() {
	mReceiverFromDcc = new CommunicationReceiver("DenService", "5555", "DENM");
	mSenderToDcc = new CommunicationSender("DenService", "7777");
	mSenderToLdm = new CommunicationSender("DenService", "9999");

	mLogger = new LoggingUtility("DenService");

	mIdCounter = 0;
}

DenService::~DenService() {
	mThreadReceive->join();
	mThreadSend->join();
}

void DenService::init() {
	mThreadReceive = new boost::thread(&DenService::receive, this);
	mThreadSend = new boost::thread(&DenService::send, this);
}

//receive DENM from DCC and forward to LDM
void DenService::receive() {
	string envelope;		//envelope
	string byteMessage;		//byte string (serialized)
	wrapperPackage::WRAPPER wrapper;

	while (1) {
		pair<string, string> received = mReceiverFromDcc->receive();
		envelope = received.first;
		byteMessage = received.second;

		wrapper.ParseFromString(byteMessage);	//deserialize WRAPPER
		byteMessage = wrapper.content();		//serialized DENM
		logDelay(byteMessage);

		cout << "forward incoming DENM to LDM" << endl;
		mSenderToLdm->send(envelope, byteMessage);
	}
}

//log delay of received DENM
void DenService::logDelay(string byteMessage) {
	denmPackage::DENM denm;
	denm.ParseFromString(byteMessage);
	int64_t createTime = denm.createtime();
	int64_t receiveTime = chrono::system_clock::now().time_since_epoch() / chrono::milliseconds(1);
	int64_t delay = receiveTime - createTime;
	mLogger->logStats("DENM", denm.id(), delay);
}

//periodically generate DENMs and send to LDM and DCC
void DenService::send() {
	string byteMessage;
	denmPackage::DENM denm;
	wrapperPackage::WRAPPER wrapper;

	while (1) {
		sleep(1);
		denm = generateDenm();
		wrapper = generateWrapper(denm);
		wrapper.SerializeToString(&byteMessage);
		cout << "send new DENM to LDM and DCC" << endl;
		mSenderToLdm->send("DENM", wrapper.content());		//send serialized DENM to LDM
		mSenderToDcc->send("DENM", byteMessage);			//send serialized WRAPPER to DCC
	}
}

//generate new DENM with increasing ID and current timestamp
denmPackage::DENM DenService::generateDenm() {
	denmPackage::DENM denm;

	//create CAM
	denm.set_id(mIdCounter++);
	denm.set_content("DENM from DEN service");
	denm.set_createtime(chrono::system_clock::now().time_since_epoch() / chrono::milliseconds(1));

	return denm;
}

wrapperPackage::WRAPPER DenService::generateWrapper(denmPackage::DENM denm) {
	wrapperPackage::WRAPPER wrapper;
	string byteMessage;

	//serialize CAM
	denm.SerializeToString(&byteMessage);

	//create WRAPPER
	wrapper.set_id(denm.id());
	wrapper.set_type(wrapperPackage::WRAPPER_Type_DENM);
	wrapper.set_priority(wrapperPackage::WRAPPER_Priority_BE);

	wrapper.set_createtime(denm.createtime());
	wrapper.set_content(byteMessage);

	return wrapper;
}

int main() {
	DenService denm;
	denm.init();

	return EXIT_SUCCESS;
}
