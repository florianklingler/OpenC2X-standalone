#include "dcc.h"

using namespace std;

DCC::DCC () {
}

DCC::~DCC () {
}

int main() {
    zmq::context_t context(1);						//1 io_thread
    zmq::socket_t subscriber(context, ZMQ_SUB);
    subscriber.connect("tcp://localhost:5563");
    subscriber.setsockopt(ZMQ_SUBSCRIBE, "", 0);	//accept all messages

    while (1) {
		string message = s_recv(subscriber);		//read message contents
        
        cout << message << endl;					//print message        
    }
    
    return 0;
}
