#include "dcc.h"

#include <iostream>
#include <unistd.h>
#include <zmq.hpp>

using namespace std;

DCC::DCC() {
	ctx = new zmq::context_t(1);

	mSendSocket = new zmq::socket_t(*ctx, ZMQ_PUB);
	mSendSocket->bind("ipc:///tmp/dcc.ipc"); // TODO: This should be IPC and not TCP when DCC is also changed

	mReceiveSocket = new zmq::socket_t(*ctx, ZMQ_SUB);
	mReceiveSocket->connect("tcp://localhost:5563"); //("ipc:///dcc/pub/"); // TODO: This should be IPC and not TCP
	mReceiveSocket->setsockopt(ZMQ_SUBSCRIBE, "B", 1);
	cout << "DCC constructor!" << endl;
}

DCC::~DCC() {
	cleanUp();
	cout << "DCC destructor" << endl;
}

boost::shared_ptr<DCC> DCC::createDCC() {
	boost::shared_ptr<DCC> ptr = boost::shared_ptr<DCC>(new DCC());
	cout << "return from createDCC" << endl;
	return ptr;
}

void DCC::init() {
	mSenderThread = new boost::thread(&DCC::sendLoop, this);
	mReceiverThread = new boost::thread(&DCC::receiveLoop, this);
	cout << "DCC::init done" << endl;
}

void DCC::start() {
	// loop();
	cout << "empty start" << endl;
}

void DCC::loop() {
	zmq::context_t context(1);
	zmq::socket_t publisher(context, ZMQ_PUB);
	publisher.bind("tcp://*:5563");
	while (1) {
		cout << "sending signal A!\n";
		s_sendmore(publisher, "A");
		s_send(publisher, "We don't want to see this");
		cout << "sending signal B!\n";
		s_sendmore(publisher, "B");
		s_send(publisher, "We would like to see this");
		cout << endl;
		sleep(1);
	}
}

void DCC::sendLoop() {
	cout << "sendLoop" << endl;
	while (1) {
		cout << "sending signal A!\n";
		s_sendmore(*mSendSocket, "A");
		s_send(*mSendSocket, "We don't want to see this");
		cout << "sending signal B!\n";
		s_sendmore(*mSendSocket, "B");
		s_send(*mSendSocket, "We would like to see this");
		cout << endl;
		sleep(1);
	}
}

void DCC::receiveLoop() {
	cout << "receiveLoop" << endl;
	while (1) {
		cout << "TODO: DCC receiveLoop" << endl;
		sleep(4);
	}
}

void DCC::cleanUp() {
	// TODO: clean up zmq! Also call clean up from installed signal handlers
	mSenderThread->join();
	mReceiverThread->join();
	cout << "DCC::cleanUp" << endl;
}
