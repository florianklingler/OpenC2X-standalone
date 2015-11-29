#include "cam.h"

CAM::CAM() {
}

CAM::~CAM() {
}

int main() {
	//pepare context and publisher
    zmq::context_t context(1);
    zmq::socket_t publisher(context, ZMQ_PUB);
    publisher.bind("tcp://*:5563");

    while (1) {
        //difference sendmore/send?			
        s_send (publisher, "Cooperative Awareness Message");	//content
        sleep (1);
    }
    return 0;
}
