#include "dcc.h"
#include <unistd.h>
#include <string>
#include <iostream>
#include <boost/thread.hpp>

using namespace std;
using namespace zmq;

DCC::DCC () {
	mReceiverFromCa = new CommunicationReceiver("6666", "CAM");
	mReceiverFromDen = new CommunicationReceiver("7777", "DENM");
	mSenderToLower = new CommunicationSender("4444");

	mCommunicationLowerToUpper = new Communication("4444", "5555", "", this);
}

DCC::~DCC () {
	mReceiveFromCaThread->join();
	mReceiveFromDenThread->join();
	mReceiveFromLowerThread->join();
}

void DCC::init() {
	mReceiveFromCaThread = new boost::thread(&DCC::receiveLoopFromCa, this);
	mReceiveFromDenThread = new boost::thread(&DCC::receiveLoopFromDen, this);
	mReceiveFromLowerThread = new boost::thread(&Communication::run, mCommunicationLowerToUpper);
}

void DCC::receiveLoopFromCa() {
	while(1) {

		pair<string, string> result = mReceiverFromCa->receive();
		//processing...
		cout << "received new CAM and forward to HW" << endl;
		mSenderToLower->send(result.first, result.second);
	}
}

void DCC::receiveLoopFromDen() {
	while(1) {

		pair<string, string> result = mReceiverFromDen->receive();
		//processing...
		cout << "received new DENM and forward to HW" << endl;
		mSenderToLower->send(result.first, result.second);
	}
}


string DCC::process(string message) {
	cout << "forward message from HW to upper" << endl;
	return message;
}

int main () {
  DCC dcc;
  dcc.init();

  return EXIT_SUCCESS;
}
