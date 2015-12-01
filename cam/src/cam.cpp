#include "cam.h"
#include "../../common/zhelpers.hpp"
#include "../../common/buffers/cam.pb.h"
#include <zmq.hpp>
#include <unistd.h>
#include <string>
#include <iostream>
#include <google/protobuf/text_format.h>

using namespace std;
using namespace zmq;

void CAM::loop () {
	context_t context (1);
	//subscriber for receiving CAMs from DCC
	socket_t subscriber_dcc (context, ZMQ_SUB);
	subscriber_dcc.connect ("tcp://localhost:5555");
	subscriber_dcc.setsockopt ( ZMQ_SUBSCRIBE, "CAM", 1);
  
  	//publisher for sending CAMs to DCC
	socket_t publisher_dcc(context, ZMQ_PUB);
	publisher_dcc.bind("tcp://*:6666");

  	//publisher for sending CAMs to LDM
	socket_t publisher_ldm(context, ZMQ_PUB);
	publisher_ldm.bind("tcp://*:8888");

	//variables
	string topic;	
	string msg_str;
	string text_str;

  	//create CAM
  	GOOGLE_PROTOBUF_VERIFY_VERSION;
  	buffers::CAM msg_cam_send; 
  	buffers::CAM msg_cam_recv;
	msg_cam_send.set_id(2345);
	msg_cam_send.set_content("CAM from CAM service");

	while (1) {
		sleep(3);

		//Send CAM to DCC
		msg_cam_send.SerializeToString(&msg_str);
		message_t request (msg_str.size());
		memcpy ((void *) request.data (), msg_str.c_str(), msg_str.size());
		cout << "Sending CAM to DCC and LDM" << endl;
		s_sendmore(publisher_dcc, "CAM");
		s_send(publisher_dcc, msg_str);
		//Send CAM to LDM
		s_sendmore(publisher_ldm, "CAM");
		s_send(publisher_ldm, msg_str);
		sleep(3);

		
		//Receive CAM from DCC
		topic = s_recv(subscriber_dcc);
		msg_str = s_recv(subscriber_dcc);
		cout << "Received CAM from DCC" << endl;
		msg_cam_recv.ParseFromString(msg_str);
		google::protobuf::TextFormat::PrintToString(msg_cam_recv, &text_str);
		cout << text_str << endl;
		//Forward DENM to LDM
		cout << "Forwarding CAM to LDM" << endl;
		s_sendmore(publisher_ldm, topic);
		s_send(publisher_ldm, msg_str);

		sleep(3);
	}
}

int main () {
	CAM cam;
	cam.loop();

	return EXIT_SUCCESS;
}
