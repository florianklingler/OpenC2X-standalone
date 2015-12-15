#include "caservice.h"

#include <buffers/build/cam.pb.h>
#include <google/protobuf/text_format.h>
#include <unistd.h>
#include <iostream>
#include <ctime>
#include <chrono>

INITIALIZE_EASYLOGGINGPP


CaService::CaService() {
	mReceiverFromDcc = new CommunicationReceiver("5555", "CAM");
	mSenderToDcc = new CommunicationSender("6666");
	mSenderToLdm = new CommunicationSender("8888");

	mLogger = new LoggingUtility();

	mIdCounter = 0;
}

CaService::~CaService() {
	mThreadReceive->join();
	mThreadSend->join();
}

void CaService::init() {
	mThreadReceive = new boost::thread(&CaService::receive, this);
	mThreadSend = new boost::thread(&CaService::send, this);
}

//receive CAM from DCC and forward to LDM
void CaService::receive() {
	string envelope;		//envelope
	string byteMessage;		//byte string (serialized CAM)
	while (1) {
		pair<string, string> received = mReceiverFromDcc->receive();
		envelope = received.first;
		byteMessage = received.second;

		logDelay(byteMessage);

		cout << "forward incoming CAM to LDM" << endl;
		mSenderToLdm->send(envelope, byteMessage);
	}
}

//log delay of received CAM
void CaService::logDelay(string byteMessage) {
	camPackage::CAM cam;
	cam.ParseFromString(byteMessage);
	int64_t createTime = cam.createtime();
	int64_t receiveTime = chrono::system_clock::now().time_since_epoch() / chrono::milliseconds(1);
	int64_t delay = receiveTime - createTime;
	mLogger->logStats("CAM", cam.id(), delay);
}

//periodically generate CAMs and send to LDM and DCC
void CaService::send() {
	string byteMessage;
	while (1) {
		sleep(1);
		byteMessage = generateCam();
		cout << "send new CAM to LDM and DCC" << endl;
		mSenderToLdm->send("CAM", byteMessage);
		mSenderToDcc->send("CAM", byteMessage);
	}
}

//generate and serialize new CAM with increasing ID and current timestamp
string CaService::generateCam() {
	camPackage::CAM message;
	string byteMessage;
	message.set_id(mIdCounter++);
	message.set_content("CAM from CA service");
	message.set_createtime(chrono::system_clock::now().time_since_epoch() / chrono::milliseconds(1));
	message.SerializeToString(&byteMessage);

	return byteMessage;
}

int main() {
	CaService cam;
	cam.init();

	return EXIT_SUCCESS;
}
