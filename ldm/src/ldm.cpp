#include "ldm.h"
#include <unistd.h>
#include <string>
#include <iostream>
#include <buffers/build/cam.pb.h>
#include <buffers/build/denm.pb.h>
#include <google/protobuf/text_format.h>

using namespace std;
//using namespace zmq;


LDM::LDM() {
	mReceiverFromCa = new CommunicationReceiver("8888", "CAM");
	mReceiverFromDen = new CommunicationReceiver("9999", "DENM");
}

LDM::~LDM() {
	mReceiveFromCaThread->join();
	mReceiveFromDenThread->join();
}

void LDM::init() {
	mReceiveFromCaThread = new boost::thread(&LDM::receiveLoopFromCa, this);
	mReceiveFromDenThread = new boost::thread(&LDM::receiveLoopFromDen, this);
}

string LDM::process(string message) {
	return message;
}

void LDM::receiveLoopFromCa() {
	string envelope;		//envelope
	string byteMessage;		//byte string (serialized CAM)
	string textMessage;		//text string (human readable)

	camPackage::CAM cam;

	while (1) {
		pair<string, string> received = mReceiverFromCa->receive();	//receive
		envelope = received.first;
		byteMessage = received.second;
		cout << "receiving CAM" << endl;
		
		//print CAM
		cam.ParseFromString(byteMessage);
		google::protobuf::TextFormat::PrintToString(cam, &textMessage);
		cout << textMessage << endl;
	}
}

void LDM::receiveLoopFromDen() {
	string envelope;		//envelope
	string byteMessage;		//byte string (serialized DENM)
	string textMessage;		//text string (human readable)

	denmPackage::DENM denm;

	while (1) {
		pair<string, string> received = mReceiverFromDen->receive();	//receive
		envelope = received.first;
		byteMessage = received.second;
		cout << "receiving DENM" << endl;

		//print DENM
		denm.ParseFromString(byteMessage);
		google::protobuf::TextFormat::PrintToString(denm, &textMessage);
		cout << textMessage << endl;
	}
}

int main () {
	LDM ldm;
	ldm.init();

	return EXIT_SUCCESS;
}
