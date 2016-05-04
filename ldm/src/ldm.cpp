#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include "ldm.h"
#include <unistd.h>
#include <string>
#include <iostream>
#include <buffers/build/cam.pb.h>
#include <buffers/build/denm.pb.h>
#include <buffers/build/gps.pb.h>
#include <google/protobuf/text_format.h>

using namespace std;

INITIALIZE_EASYLOGGINGPP

LDM::LDM() {
	mReceiverFromCa = new CommunicationReceiver("Ldm", "8888", "CAM");
	mReceiverFromDen = new CommunicationReceiver("Ldm", "9999", "DENM");
	mLogger = new LoggingUtility("LDM");
}

LDM::~LDM() {
	mThreadReceiveFromCa->join();
	mThreadReceiveFromDen->join();
	delete mThreadReceiveFromCa;
	delete mThreadReceiveFromDen;

	delete mReceiverFromCa;
	delete mReceiverFromDen;
}

void LDM::init() {
	mThreadReceiveFromCa = new boost::thread(&LDM::receiveFromCa, this);
	mThreadReceiveFromDen = new boost::thread(&LDM::receiveFromDen, this);
}

void LDM::receiveFromCa() {
	string serializedCam;	//serialized CAM
	string textMessage;		//text string (human readable)

	camPackage::CAM cam;

	while (1) {
		pair<string, string> received = mReceiverFromCa->receive();	//receive
		serializedCam = received.second;

		//print CAM
		cam.ParseFromString(serializedCam);
		google::protobuf::TextFormat::PrintToString(cam, &textMessage);
		mLogger->logInfo("received CAM:\n" + textMessage);
	}
}

void LDM::receiveFromDen() {
	string serializedDenm;		//serialized DENM
	string textMessage;		//text string (human readable)

	denmPackage::DENM denm;

	while (1) {
		pair<string, string> received = mReceiverFromDen->receive();//receive
		serializedDenm = received.second;

		//print DENM
		denm.ParseFromString(serializedDenm);
		google::protobuf::TextFormat::PrintToString(denm, &textMessage);
		mLogger->logInfo("received DENM:\n" + textMessage);
	}
}

int main() {
	LDM ldm;
	ldm.init();

	return EXIT_SUCCESS;
}
