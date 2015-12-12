#include "dcc.h"
#include <unistd.h>
#include <string>
#include <iostream>

using namespace std;

DCC::DCC() {
	mReceiverFromCa = new CommunicationReceiver("6666", "CAM");
	mReceiverFromDen = new CommunicationReceiver("7777", "DENM");
	mReceiverFromHw = new CommunicationReceiver("4444", "");
	mSenderToHw = new CommunicationSender("4444");
	mSenderToServices = new CommunicationSender("5555");
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
		mSenderToHw->send(envelope, byteMessage);
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
		mSenderToHw->send(envelope, byteMessage);
	}
}

void DCC::receiveFromHw() {
	string envelope;		//envelope
	string byteMessage;		//byte string (serialized message)
	while (1) {
		pair<string, string> received = mReceiverFromHw->receive();
		envelope = received.first;
		byteMessage = received.second;

		//processing...
		cout << "forward message from HW to services" << endl;
		mSenderToServices->send(envelope, byteMessage);
	}
}

int main() {
	DCC dcc;
	dcc.init();

	return EXIT_SUCCESS;
}
