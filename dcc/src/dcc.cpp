
#include "dcc.h"
#include "../../common/utility/zhelpers.hpp"

#include <buffers/build/cam.pb.h> //just for output
#include <buffers/build/cam.pb.cc> //ugly but works
//#include "../../common/buffers/build/denm.pb.h"
#include <buffers/build/denm.pb.h>
#include <buffers/build/denm.pb.cc> //ugly but works

#include <unistd.h>
#include <zmq.hpp>
#include <string>
#include <iostream>
#include <google/protobuf/text_format.h>
#include <boost/thread.hpp>



//#include "../../common/buffers/build/cam.pb.h" //just for output
//#include "../../common/buffers/messages/messages.pb.h"
//#include "../../common/buffers/messages/messages.pb.cc"



using namespace std;
using namespace zmq;

DCC::DCC () {
	cout << "constuctorBeginn" << endl;
	
	context= new zmq::context_t(1);
	
	//publisher for sending CAM/DENMs up
	publisher_up = new socket_t(*context, ZMQ_PUB);
	publisher_up->bind("tcp://*:5555");

	//publisher for sending CAM/DENMs down
	publisher_down = new socket_t(*context, ZMQ_PUB);
	publisher_down->bind("tcp://*:4444");
	
	//subscriber for receiving CAM/DENMs from top
	subscriber_up  = new socket_t(*context, ZMQ_SUB);
	subscriber_up->connect ("tcp://localhost:6666"); //CAM
	subscriber_up->setsockopt ( ZMQ_SUBSCRIBE, "CAM", 1);
	subscriber_up->connect ("tcp://localhost:7777"); //DENM
	subscriber_up->setsockopt ( ZMQ_SUBSCRIBE, "DENM", 1);

	//subscriber for receiving CAM/DENMs from below
	subscriber_down= new socket_t(*context, ZMQ_SUB);
	subscriber_down->connect ("tcp://localhost:4444");	//callback
	subscriber_down->setsockopt ( ZMQ_SUBSCRIBE, "", 0);
	
	cout << "constuctorEnd" << endl;
}

DCC::~DCC () {
	receiveFromUpperThread->join();
}

void DCC::init() {
	receiveFromUpperThread = new boost::thread(&DCC::receiveLoopFromUpper, this);
	receiveFromLowerThread = new boost::thread(&DCC::receiveLoopFromLower, this);
	
	cout << "init" << endl;
}


void DCC::receiveLoopFromUpper() {
	GOOGLE_PROTOBUF_VERIFY_VERSION;
  	//variables
	string topic;	
	string msg_str;
	string text_str;

<<<<<<< HEAD
	CAM_PACKAGE::CAM msg_cam_recv;
	DENM_PACKAGE::DENM msg_denm_recv;

=======
	buffers::CAM msg_cam_recv;
	buffers::DENM msg_denm_recv;
	
>>>>>>> 0eb70e80f7c22234620f393abf2c32a67882e585
	while (1) {
		cout << "receiveUpper" << endl;
		//Receive CAM/DENM from CAM/DENM service
		topic = s_recv(*subscriber_up);
		msg_str = s_recv(*subscriber_up);
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
		s_sendmore(*publisher_down, topic);
		s_send(*publisher_down, msg_str);

		sleep(1);
	}
}


void DCC::receiveLoopFromLower() {
	GOOGLE_PROTOBUF_VERIFY_VERSION;
  	//variables
	string topic;	
	string msg_str;
	string text_str;

	buffers::CAM msg_cam_recv;
	buffers::DENM msg_denm_recv;	

	while (1) {
		cout << "receiveLower" << endl;
		//Receive CAM/DENM from below
		topic = s_recv(*subscriber_down);
		msg_str = s_recv(*subscriber_down);
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
		s_sendmore(*publisher_up, topic);
		s_send(*publisher_up, msg_str);

		sleep(1);
    }
}

int main () {
  DCC dcc;
  dcc.init();

  return EXIT_SUCCESS;
}
