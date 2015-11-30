#include "cam.h"
#include "../../lib/msgs.pb.h"

using namespace std;

CAM::CAM() {
}

CAM::~CAM() {
}

int main() {
	GOOGLE_PROTOBUF_VERIFY_VERSION;	//check compability of library/header

	//pepare context and publisher
    zmq::context_t context(1);
    zmq::socket_t publisher(context, ZMQ_PUB);
    publisher.bind("tcp://*:5563");
    
    msgs::CAM cam;	//create CAM message
    int id = 0;

    while (1) {
        //TODO: difference sendmore/send?		
        cam.set_id(id);	
        cout << "Created CAM with ID " << id << endl;
        //s_send (publisher, "Cooperative Awareness Message");	//content
        sleep (1);
        id++;
        
    }
    return 0;
}
