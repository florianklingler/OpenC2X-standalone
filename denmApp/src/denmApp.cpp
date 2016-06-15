#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include "denmApp.h"

using namespace std;

INITIALIZE_EASYLOGGINGPP

DenmApp::DenmApp() {
	mSenderToDenm = new CommunicationSender("DenmApp", "1111");
}

DenmApp::~DenmApp() {
	delete mSenderToDenm;
}

void DenmApp::triggerDenm(string content) {
	triggerPackage::TRIGGER trigger;
	string serializedTrigger;

	trigger.set_content(content);

	trigger.SerializeToString(&serializedTrigger);
	mSenderToDenm->send("TRIGGER", serializedTrigger);
}



int main(int argc, const char* argv[]) {
	DenmApp denmApp;

	usleep(200*1000);	//FIXME: zmq seems to need some time for setup => doesn't send without sleep

	if (argc >= 2) {
		string content(argv[1]);
		denmApp.triggerDenm(content);
	}
	else {
		cout << "Missing arguments" << endl;
	}

	exit(0);			//FIXME: CommunicationSender doesn't terminate => program doesn't quit without exit

	return 0;
}
