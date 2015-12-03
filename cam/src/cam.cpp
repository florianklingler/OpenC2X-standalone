#include "cam.h"
#include <utility/zhelpers.hpp>
#include <buffers/build/cam.pb.h>
#include <buffers/build/cam.pb.cc> //ugly but works
#include <zmq.hpp>
#include <unistd.h>
#include <string>
#include <iostream>
#include <google/protobuf/text_format.h>
#include <boost/thread.hpp>

using namespace std;
using namespace zmq;

CAM::CAM () {
	context = new context_t(1);
	//subscriber for receiving CAMs from DCC
	subscriber_dcc = new socket_t(*context, ZMQ_SUB);
	subscriber_dcc->connect ("tcp://localhost:5555");
	subscriber_dcc->setsockopt (ZMQ_SUBSCRIBE, "CAM", 1);
  
  	//publisher for sending CAMs to DCC
	publisher_dcc = new socket_t(*context, ZMQ_PUB);
	publisher_dcc->bind("tcp://*:6666");

  	//publisher for sending CAMs to LDM
	publisher_ldm = new socket_t(*context, ZMQ_PUB);
	publisher_ldm->bind("tcp://*:8888");
}

CAM::~CAM () {
	receiveFromDCCThread->join();
	sendThread->join();
}

void CAM::init(){
	receiveFromDCCThread = new boost::thread(&CAM::receiveFromDCCLoop, this);
	sendThread = new boost::thread(&CAM::sendLoop, this);
}

void CAM::receiveFromDCCLoop() {
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	//variables
	string topic;	
	string msg_str;
	string text_str;

  	//create CAM
  	camPackage::CAM msg_cam_recv;


	while (1) {
		//Receive CAM from DCC
		topic = s_recv(*subscriber_dcc);
		msg_str = s_recv(*subscriber_dcc);
		cout << "Received CAM from DCC" << endl;
		msg_cam_recv.ParseFromString(msg_str);
		google::protobuf::TextFormat::PrintToString(msg_cam_recv, &text_str);
		cout << text_str << endl;
		
		//Forward DENM to LDM
		cout << "Forwarding CAM to LDM" << endl;
		s_sendmore(*publisher_ldm, topic);
		s_send(*publisher_ldm, msg_str);
	}
}

void CAM::sendLoop() {
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	//variables
	string topic;	
	string msg_str;
	string text_str;

  	//create CAM
	camPackage::CAM msg_cam_send;
	msg_cam_send.set_id(2345);
	msg_cam_send.set_content("CAM from CAM service");

	while (1) {
		//Send CAM to DCC
		msg_cam_send.SerializeToString(&msg_str);
		message_t request (msg_str.size());
		memcpy ((void *) request.data (), msg_str.c_str(), msg_str.size());
		cout << "Sending CAM to DCC and LDM" << endl;
		s_sendmore(*publisher_dcc, "CAM");
		s_send(*publisher_dcc, msg_str);
		//Send CAM to LDM
		s_sendmore(*publisher_ldm, "CAM");
		s_send(*publisher_ldm, msg_str);
		sleep(3);
	}
}

int main () {
	CAM cam;
	cam.init();

	return EXIT_SUCCESS;
}
