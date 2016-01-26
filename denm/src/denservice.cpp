#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include "denservice.h"
#include <google/protobuf/text_format.h>
#include <unistd.h>
#include <iostream>
#include <ctime>
#include <chrono>
#include <string>
#include <stdlib.h>

using namespace std;

INITIALIZE_EASYLOGGINGPP

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
	mThreadSend = new boost::thread(&DenService::triggerDenm, this);
}

//receive DENM from DCC and forward to LDM
void DenService::receive() {
	string envelope;		//envelope
	string byteMessage;		//byte string (serialized)
	dataPackage::DATA data;

	while (1) {
		pair<string, string> received = mReceiverFromDcc->receive();
		envelope = received.first;
		byteMessage = received.second;

		data.ParseFromString(byteMessage);	//deserialize DATA
		byteMessage = data.content();		//serialized DENM
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
	int64_t receiveTime =
			chrono::high_resolution_clock::now().time_since_epoch()
					/ chrono::nanoseconds(1);
	int64_t delay = receiveTime - createTime;
	mLogger->logStats("DENM", denm.id(), delay);
}

//trigger sending to LDM and DCC every 100 to 1000ms to simulate road safety application
void DenService::triggerDenm() {
	while (1) {
		int randomSleep = rand() % 900 + 101;
		microSleep(randomSleep*1000);
		send();
	}
}

void DenService::microSleep(double microSeconds) {
	time_t sleep_sec = (time_t) (((int) microSeconds) / (1000 * 1000));
	long sleep_nanosec = ((long) (microSeconds * 1000)) % (1000 * 1000 * 1000);

	struct timespec time[1];
	time[0].tv_sec = sleep_sec;
	time[0].tv_nsec = sleep_nanosec;
	nanosleep(time, NULL);
}

//generate DENM and send to LDM and DCC
void DenService::send() {
	string byteMessage;
	denmPackage::DENM denm;
	dataPackage::DATA data;

	denm = generateDenm();
	data = generateData(denm);
	data.SerializeToString(&byteMessage);
	cout << "send new DENM to LDM and DCC" << endl;
	mSenderToLdm->send("DENM", data.content());		//send serialized DENM to LDM
	mSenderToDcc->send("DENM", byteMessage);		//send serialized DATA to DCC
}

//generate new DENM with increasing ID and current timestamp
denmPackage::DENM DenService::generateDenm() {
	denmPackage::DENM denm;

	//create DENM
	denm.set_id(mIdCounter++);
	denm.set_content("DENM from DEN service");
	denm.set_createtime(chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1));

	return denm;
}

dataPackage::DATA DenService::generateData(denmPackage::DENM denm) {
	dataPackage::DATA data;
	string byteMessage;

	//serialize DENM
	denm.SerializeToString(&byteMessage);

	//create DATA
	data.set_id(denm.id());
	data.set_type(dataPackage::DATA_Type_DENM);
	data.set_priority(dataPackage::DATA_Priority_BE);

	data.set_createtime(denm.createtime());
	data.set_validuntil(denm.createtime() + 1*1000*1000*1000);	//1s
	data.set_content(byteMessage);

	return data;
}

int main() {
	DenService denm;
	denm.init();

	return EXIT_SUCCESS;
}
