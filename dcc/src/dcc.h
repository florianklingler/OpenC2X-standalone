#include <zmq.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

class DCC {
public:
	DCC ();
	~DCC ();
	
	void init();
	void loop();
	void receiveLoopFromUpper();
	void receiveLoopFromLower();

private:
	zmq::context_t* context;
	zmq::socket_t* publisher_up;
	zmq::socket_t* publisher_down;
	zmq::socket_t* subscriber_up;
	zmq::socket_t* subscriber_down;

	boost::thread* receiveFromUpperThread;
	boost::thread* receiveFromLowerThread;
};
