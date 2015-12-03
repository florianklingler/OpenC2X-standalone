#include "ldm.h"
#include <utility/zhelpers.hpp>
#include <buffers/build/cam.pb.h> //just for output
#include <buffers/build/cam.pb.cc> //ugly but works
#include <buffers/build/denm.pb.h>
#include <buffers/build/denm.pb.cc> //ugly but works
#include <unistd.h>
#include <string>
#include <iostream>
#include <google/protobuf/text_format.h>

using namespace std;
using namespace zmq;


LDM::LDM() {
	GOOGLE_PROTOBUF_VERIFY_VERSION;	
	
	context = new context_t(1);
	
	//subscriber for receiving CAM/DENMs from CAM/DENM service
	subscriber = new socket_t(*context, ZMQ_SUB);
	subscriber->connect("tcp://localhost:8888"); //CAM
	subscriber->setsockopt( ZMQ_SUBSCRIBE, "CAM", 1);
	subscriber->connect("tcp://localhost:9999"); //DENM
	subscriber->setsockopt(ZMQ_SUBSCRIBE, "DENM", 1);
}

LDM::~LDM() {
	receiveThread->join();
}

void LDM::init() {
	receiveThread = new boost::thread(&LDM::receiveLoop, this);
}

void LDM::receiveLoop() {
  	//variables
	string topic;		//envelope
	string msg_str;		//byte string
	string text_str;	//text string

	CAM_PACKAGE::CAM msg_cam_recv;
	DENM_PACKAGE::DENM msg_denm_recv;

	while (1) {
		//Receive CAM/DENM from CAM/DENM service
		topic = s_recv(*subscriber);
		msg_str = s_recv(*subscriber);
		if(topic == "CAM") {
			cout << "Received CAM from CAM service" << endl;
			msg_cam_recv.ParseFromString(msg_str);
			google::protobuf::TextFormat::PrintToString(msg_cam_recv, &text_str);
			cout << text_str << endl;
		}
		if(topic == "DENM") {
			cout << "Received DENM from DENM service" << endl;
			msg_denm_recv.ParseFromString(msg_str);
			google::protobuf::TextFormat::PrintToString(msg_denm_recv, &text_str);
			cout << text_str << endl;
		}
		//sleep(1);
	}
}

int main () {
	LDM ldm;
	ldm.init();

	return EXIT_SUCCESS;
}
