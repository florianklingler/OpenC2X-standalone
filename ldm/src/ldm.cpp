#include "ldm.h"
#include "../../common/zhelpers.hpp"
#include "../../common/buffers/cam.pb.h"
#include "../../common/buffers/denm.pb.h"
#include <zmq.hpp>
#include <unistd.h>
#include <string>
#include <iostream>
#include <google/protobuf/text_format.h>

using namespace std;
using namespace zmq;

void LDM::loop () {
	//subscriber for receiving CAM/DENMs from CAM/DENM service
	context_t context(1);
	socket_t subscriber (context, ZMQ_SUB);
	subscriber.connect ("tcp://localhost:8888"); //CAM
	subscriber.setsockopt ( ZMQ_SUBSCRIBE, "CAM", 1);
	subscriber.connect ("tcp://localhost:9999"); //DENM
	subscriber.setsockopt ( ZMQ_SUBSCRIBE, "DENM", 1);
	
	GOOGLE_PROTOBUF_VERIFY_VERSION;
  	//variables
	string topic;	
	string msg_str;
	string text_str;

	buffers::CAM msg_cam_recv;
	buffers::DENM msg_denm_recv;

	while (1) {

		//Receive CAM/DENM from CAM/DENM service
		topic = s_recv(subscriber);
		msg_str = s_recv(subscriber);
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
		sleep(1);
	}
}

int main () {
	LDM ldm;
	ldm.loop();

	return EXIT_SUCCESS;
}
