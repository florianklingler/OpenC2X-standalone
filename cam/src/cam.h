#include <zmq.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

class CAM {
public:
	CAM ();
	~CAM ();
	
	void init();
	void receiveFromDCCLoop();
	void sendLoop();

private:
	zmq::context_t* context;
	zmq::socket_t* publisher_dcc;
	zmq::socket_t* publisher_ldm;
	zmq::socket_t* subscriber_dcc;

	boost::thread* receiveFromDCCThread;
	boost::thread* sendThread;
};
