#include "denm.h"
#include "../../common/zhelpers.hpp"
#include "../../common/buffers/denm.pb.h"
#include <zmq.hpp>
#include <unistd.h>
#include <string>
#include <iostream>
#include <google/protobuf/text_format.h>

using namespace std;
using namespace zmq;

void DENM::loop () {
	context_t context (1);
  	//subscriber for receiving DENMs from DCC
  	socket_t subscriber_dcc (context, ZMQ_SUB);
	subscriber_dcc.connect ("tcp://localhost:5555");
  	subscriber_dcc.setsockopt ( ZMQ_SUBSCRIBE, "DENM", 1);
  
  	//publisher for sending DENMs to DCC
  	socket_t publisher_dcc(context, ZMQ_PUB);
  	publisher_dcc.bind("tcp://*:7777");

  	//publisher for sending DENMs to LDM
	socket_t publisher_ldm(context, ZMQ_PUB);
	publisher_ldm.bind("tcp://*:9999");

  	//variables
  	string topic;	
  	string msg_str;
  	string text_str;

  	//create DENM
  	GOOGLE_PROTOBUF_VERIFY_VERSION;
  	buffers::DENM msg_denm_send; 
  	buffers::DENM msg_denm_recv;
  	msg_denm_send.set_id(2345);
  	msg_denm_send.set_content("DENM from DENM service");

  	while (1) {
		sleep(3);

		//Send DENM to DCC
		msg_denm_send.SerializeToString(&msg_str);
		message_t request (msg_str.size());
		memcpy ((void *) request.data (), msg_str.c_str(), msg_str.size());
		cout << "Sending DENM to DCC and LDM" << endl;
		s_sendmore(publisher_dcc, "DENM");
		s_send(publisher_dcc, msg_str);
		//Send DENM to LDM
		s_sendmore(publisher_ldm, "DENM");
		s_send(publisher_ldm, msg_str);
		sleep(3);
		

    		//Receive DENM from DCC
		topic = s_recv(subscriber_dcc);
		msg_str = s_recv(subscriber_dcc);
        	cout << "Received DENM from DCC" << endl;
        	msg_denm_recv.ParseFromString(msg_str);
        	google::protobuf::TextFormat::PrintToString(msg_denm_recv, &text_str);
        	cout << text_str << endl;
		//Forward DENM to LDM
		cout << "Forwarding DENM to LDM" << endl;
		s_sendmore(publisher_ldm, topic);
		s_send(publisher_ldm, msg_str);

		sleep(3);
  	}
}

int main () {
  	DENM denm;
  	denm.loop();

  	return EXIT_SUCCESS;
}
