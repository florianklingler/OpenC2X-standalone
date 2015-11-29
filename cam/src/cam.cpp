#include "cam.h"
#include "zhelpers.hpp"
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <unistd.h>
#include <iostream>
#include <memory>

using namespace std;

CAM::CAM() {
	ctx = new zmq::context_t(1);

	mSendSocket = new zmq::socket_t(*ctx, ZMQ_PUB);
	mSendSocket->connect("tcp://localhost:5563"); //("ipc:///dcc/sub/"); // TODO: This should be IPC and not TCP when DCC is also changed

	mReceiveSocket = new zmq::socket_t(*ctx, ZMQ_SUB);
	mReceiveSocket->connect("ipc:///tmp/dcc.ipc"); // TODO: This should be IPC and not TCP
	mReceiveSocket->setsockopt(ZMQ_SUBSCRIBE, "B", 1);
	cout << "CAM constructor!" << endl;
}

CAM::~CAM() {
	cleanUp();
	cout << "CAM destructor" << endl;
}

boost::shared_ptr<CAM> CAM::createCAM() {
	boost::shared_ptr<CAM> ptr = boost::shared_ptr<CAM>(new CAM());
	cout << "return from createCAM" << endl;
	return ptr;
}

void CAM::init() {
	mSenderThread = new boost::thread(&CAM::sendLoop, this);
	mReceiverThread = new boost::thread(&CAM::receiveLoop, this);
	cout << "CAM::init done" << endl;
}

void CAM::start() {
	//loop();
	cout << "empty start" << endl;
}

void CAM::loop() {
	//  Prepare our context and subscriber
	zmq::context_t context(1);
	zmq::socket_t subscriber(context, ZMQ_SUB);
	subscriber.connect("tcp://localhost:5563");
	subscriber.setsockopt( ZMQ_SUBSCRIBE, "B", 1);

	while (1) {
		//  Read envelope with address
		std::string address = s_recv(subscriber);
		//  Read message contents
		std::string contents = s_recv(subscriber);
		std::cout << "[" << address << "] " << contents << std::endl;
	}
}

void CAM::sendLoop() {
	cout << "sendLoop" << endl;
	while (1) {
		cout << "TODO: CAM sendLoop"<<endl;
		sleep(4);
	}
}

void CAM::receiveLoop() {
	cout << "receiveLoop" << endl;
	while (1) {
		//  Read envelope with address
		string address = s_recv(*mReceiveSocket);
		//  Read message contents
		string contents = s_recv(*mReceiveSocket);
		cout << "[" << address << "] " << contents << endl;
	}
}

void CAM::cleanUp() {
	// TODO: clean up zmq! Also call clean up from installed signal handlers
	mSenderThread->join();
	mReceiverThread->join();
	cout << "cleanUp" << endl;
}
