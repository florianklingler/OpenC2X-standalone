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
}

LDM::~LDM() {
	mThreadReceiveFromCa->join();
	mThreadReceiveFromDen->join();
}

void LDM::init() {
	mThreadReceiveFromCa = new boost::thread(&LDM::receiveFromCa, this);
	mThreadReceiveFromDen = new boost::thread(&LDM::receiveFromDen, this);
}

void LDM::receiveFromCa() {
	string envelope;		//envelope
	string byteMessage;		//byte string (serialized CAM)
	string textMessage;		//text string (human readable)

	camPackage::CAM cam;

	while (1) {
		pair<string, string> received = mReceiverFromCa->receive();	//receive
		envelope = received.first;
		byteMessage = received.second;
		cout << "received CAM" << endl;

		//print CAM
		cam.ParseFromString(byteMessage);
		google::protobuf::TextFormat::PrintToString(cam, &textMessage);
		cout << textMessage << endl;
	}
}

void LDM::receiveFromDen() {
	string envelope;		//envelope
	string byteMessage;		//byte string (serialized DENM)
	string textMessage;		//text string (human readable)

	denmPackage::DENM denm;

	while (1) {
		pair<string, string> received = mReceiverFromDen->receive();//receive
		envelope = received.first;
		byteMessage = received.second;
		cout << "received DENM" << endl;

		//print DENM
		denm.ParseFromString(byteMessage);
		google::protobuf::TextFormat::PrintToString(denm, &textMessage);
		cout << textMessage << endl;
	}
}

int main() {
	LDM ldm;
	ldm.init();

	return EXIT_SUCCESS;
}
