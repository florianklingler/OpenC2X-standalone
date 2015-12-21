#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include "dcc.h"
#include <unistd.h>
#include <string>
#include <iostream>
#include <buffers/build/wrapper.pb.h>

using namespace std;

INITIALIZE_EASYLOGGINGPP

DCC::DCC() {
	mReceiverFromCa = new CommunicationReceiver("Dcc", "6666", "CAM");
	mReceiverFromDen = new CommunicationReceiver("Dcc", "7777", "DENM");
	mReceiverFromHw = new CommunicationReceiver("Dcc", "4444", "");
	mSenderToHw = new CommunicationSender("Dcc", "4444");
	mSenderToServices = new CommunicationSender("Dcc", "5555");
}

DCC::~DCC() {
	mThreadReceiveFromCa->join();
	mThreadReceiveFromDen->join();
	mThreadReceiveFromHw->join();
}

void DCC::init() {
	mThreadReceiveFromCa = new boost::thread(&DCC::receiveFromCa, this);
	mThreadReceiveFromDen = new boost::thread(&DCC::receiveFromDen, this);
	mThreadReceiveFromHw = new boost::thread(&DCC::receiveFromHw, this);
}

void DCC::receiveFromCa() {
	string envelope;		//envelope
	string byteMessage;		//byte string (serialized DENM)
	while (1) {
		pair<string, string> received = mReceiverFromCa->receive();
		envelope = received.first;
		byteMessage = received.second;

		//processing...
		cout << "received new CAM and forward to HW" << endl;
		mSenderToHw->sendToHw(byteMessage);
	}
}

void DCC::receiveFromDen() {
	string envelope;		//envelope
	string byteMessage;		//byte string (serialized DENM)
	while (1) {
		pair<string, string> received = mReceiverFromDen->receive();
		envelope = received.first;
		byteMessage = received.second;

		//processing...
		cout << "received new DENM and forward to HW" << endl;
		mSenderToHw->sendToHw(byteMessage);
	}
}

void DCC::receiveFromHw() {
	string byteMessage;		//byte string (serialized message)
	wrapperPackage::WRAPPER wrapper;

	while (1) {
		byteMessage = mReceiverFromHw->receiveFromHw();		//receive serialized WRAPPER
		wrapper.ParseFromString(byteMessage);				//deserialize WRAPPER

		//processing...
		cout << "forward message from HW to services" << endl;
		switch(wrapper.type()) {							//send serialized WRAPPER to corresponding module
			case wrapperPackage::WRAPPER_Type_CAM: 		mSenderToServices->send("CAM", byteMessage);	break;
			case wrapperPackage::WRAPPER_Type_DENM:		mSenderToServices->send("DENM", byteMessage);	break;
			default:	break;
		}

	}
}

int main() {
	DCC dcc;
	dcc.init();

	return EXIT_SUCCESS;
}
