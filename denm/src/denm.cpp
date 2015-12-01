#include "denm.h"
#include "../../common/utility/zhelpers.hpp"
#include <buffers/build/denm.pb.h>
#include <buffers/build/denm.pb.cc> //ugly but works
#include <zmq.hpp>
#include <unistd.h>
#include <string>
#include <iostream>
#include <google/protobuf/text_format.h>

using namespace std;
using namespace zmq;

DENM::DENM () {
	context = new zmq::context_t(1);
  	//subscriber for receiving DENMs from DCC
  	subscriber_dcc = new zmq::socket_t(*context, ZMQ_SUB);
	subscriber_dcc->connect ("tcp://localhost:5555");
  	subscriber_dcc->setsockopt ( ZMQ_SUBSCRIBE, "DENM", 1);
  
  	//publisher for sending DENMs to DCC
  	publisher_dcc = new zmq::socket_t(*context, ZMQ_PUB);
  	publisher_dcc->bind("tcp://*:7777");

  	//publisher for sending DENMs to LDM
	publisher_ldm = new zmq::socket_t(*context, ZMQ_PUB);
	publisher_ldm->bind("tcp://*:9999");
}

DENM::~DENM() {
	mSendToDccThread->join();
	mReceiveFromDccThread->join();
}

void DENM::init() {
	mSendToDccThread = new boost::thread(&DENM::sendToDccLoop, this);
	mReceiveFromDccThread = new boost::thread(&DENM::receiveFromDccLoop, this);
}

void DENM::loop() {

}


void DENM::receiveFromDccLoop() {
	//variables
  	string topic;	
  	string msg_str;
  	string text_str;
  	
  	GOOGLE_PROTOBUF_VERIFY_VERSION;
  	DENM_PACKAGE::DENM msg_denm_recv;
  	
	while(1) {
		cout<<"Receiving DENM from DCC"<<endl;
		//Receive DENM from DCC
		topic = s_recv(*subscriber_dcc);
		msg_str = s_recv(*subscriber_dcc);
		cout << "Received DENM from DCC" << endl;
		msg_denm_recv.ParseFromString(msg_str);
		google::protobuf::TextFormat::PrintToString(msg_denm_recv, &text_str);
		cout << text_str << endl;
				
		//Forward DENM to LDM
		cout << "Forwarding DENM to LDM" << endl;
		s_sendmore(*publisher_ldm, topic);
		s_send(*publisher_ldm, msg_str);
	}
}
	
void DENM::sendToDccLoop() {
	//variables
  	string topic;	
  	string msg_str;
  	string text_str;

  	//create DENM
  	GOOGLE_PROTOBUF_VERIFY_VERSION;
  	DENM_PACKAGE::DENM msg_denm_send;
  	msg_denm_send.set_id(2345);
  	msg_denm_send.set_content("DENM from DENM service");
	while(1) {
		//Send DENM to DCC
		msg_denm_send.SerializeToString(&msg_str);
		message_t request (msg_str.size());
		memcpy ((void *) request.data (), msg_str.c_str(), msg_str.size());
		cout << "Sending DENM to DCC and LDM" << endl;
		s_sendmore(*publisher_dcc, "DENM");
		s_send(*publisher_dcc, msg_str);
		
		//Send DENM to LDM
		s_sendmore(*publisher_ldm, "DENM");
		s_send(*publisher_ldm, msg_str);
		sleep(3);
	}
}

int main () {
  	DENM denm;
  	denm.init();

  	return EXIT_SUCCESS;
}
