#include "cam.h"
#include <buffers/build/cam.pb.h>
#include <google/protobuf/text_format.h>
#include <unistd.h>
#include <string>
#include <iostream>

using namespace std;
using namespace zmq;

CAM::CAM() {
	mCommunicationDccToLdm = new Communication("5555", "8888", "CAM", this);
	mSenderDcc = new CommunicationSender("6666");
}

CAM::~CAM() {
	mToDccThread->join();
	mDccToLdmThread->join();
}

void CAM::init() {
	mToDccThread = new boost::thread(&CAM::sendTestLoop, this);
	mDccToLdmThread = new boost::thread(&Communication::run,
			mCommunicationDccToLdm);
}

string CAM::process(string message) {
	cout << "forward incoming CAM to LDM" << endl;
	return message;
}

void CAM::sendTestLoop() {
	camPackage::CAM msgCamSend;
	string msg;
	msgCamSend.set_id(12);
	msgCamSend.set_content("CAM from CA service");
	msgCamSend.SerializeToString(&msg);
	while (1) {
		sleep(1);
		cout << "send new CAM to LDM and DCC" << endl;
		mCommunicationDccToLdm->send("CAM", msg);
		mSenderDcc->send("CAM", msg);
	}
}

int main() {
	CAM cam;
	cam.init();

	return EXIT_SUCCESS;
}
