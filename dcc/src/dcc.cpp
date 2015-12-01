#include "dcc.h"
#include "../../common/zhelpers.hpp"
#include "../../common/buffers/cam.pb.h" //just for output
#include "../../common/buffers/denm.pb.h"
#include <unistd.h>
#include <zmq.hpp>
#include <string>
#include <iostream>
#include <google/protobuf/text_format.h>

using namespace std;
using namespace zmq;

DCC::DCC () {
//  context = new context_t(1);
//  publisher = new socket_t(*context, ZMQ_PUB);
//  publisher->bind("tcp://*.5563");
}

DCC::~DCC () {
//  publisher->unbind(*context);
//  delete *context;
}

void DCC::loop () {
	//publisher for sending CAM/DENMs up
	context_t context(1);
	socket_t publisher_up(context, ZMQ_PUB);
	publisher_up.bind("tcp://*:5555");

	//publisher for sending CAM/DENMs down
	socket_t publisher_down(context, ZMQ_PUB);
	publisher_down.bind("tcp://*:4444");

	//subscriber for receiving CAM/DENMs from top
	socket_t subscriber_up (context, ZMQ_SUB);
	subscriber_up.connect ("tcp://localhost:6666"); //CAM
	subscriber_up.setsockopt ( ZMQ_SUBSCRIBE, "CAM", 1);
	subscriber_up.connect ("tcp://localhost:7777"); //DENM
	subscriber_up.setsockopt ( ZMQ_SUBSCRIBE, "DENM", 1);

	//subscriber for receiving CAM/DENMs from below
	socket_t subscriber_down (context, ZMQ_SUB);
	subscriber_down.connect ("tcp://localhost:4444");	//callback
	subscriber_down.setsockopt ( ZMQ_SUBSCRIBE, "", 0);

	GOOGLE_PROTOBUF_VERIFY_VERSION;
  	//variables
	string topic;	
	string msg_str;
	string text_str;

	buffers::CAM msg_cam_recv;
	buffers::DENM msg_denm_recv;

	while (1) {

		//Receive CAM/DENM from CAM/DENM service
		topic = s_recv(subscriber_up);
		msg_str = s_recv(subscriber_up);
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

		//Send message down
		s_sendmore(publisher_down, topic);
		s_send(publisher_down, msg_str);

		sleep(1);

		//Receive CAM/DENM from below
		topic = s_recv(subscriber_down);
		msg_str = s_recv(subscriber_down);
		if(topic == "CAM") {
			cout << "Received CAM from below" << endl;
			msg_cam_recv.ParseFromString(msg_str);
			google::protobuf::TextFormat::PrintToString(msg_cam_recv, &text_str);
			cout << text_str << endl;
		}
		if(topic == "DENM") {
			cout << "Received DENM from below" << endl;
			msg_denm_recv.ParseFromString(msg_str);
			google::protobuf::TextFormat::PrintToString(msg_denm_recv, &text_str);
			cout << text_str << endl;
		}
		sleep(1);

		//Send message up
		s_sendmore(publisher_up, topic);
		s_send(publisher_up, msg_str);

		sleep(1);
    }
}

int main () {
  DCC dcc;
  dcc.loop();

  return EXIT_SUCCESS;
}
