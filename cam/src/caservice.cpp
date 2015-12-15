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

void CaService::receive() {
	string envelope;		//envelope
	string byteMessage;		//byte string (serialized CAM)
	while (1) {
		pair<string, string> received = mReceiverFromDcc->receive();
		envelope = received.first;
		byteMessage = received.second;

		cout << "forward incoming CAM to LDM" << endl;
		mSenderToLdm->send(envelope, byteMessage);
	}
}

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
