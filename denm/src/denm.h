#include <zmq.hpp>
#include <boost/thread.hpp>

class DENM
{
public:
	DENM();
	~DENM();
	void init();
	void loop();
	void receiveFromDccLoop();
	void sendToDccLoop();
	void sendToLDM();
	
	
private:
	zmq::context_t* context;
	zmq::socket_t* subscriber_dcc;
	zmq::socket_t* publisher_dcc;
	zmq::socket_t* publisher_ldm;
	
	
	boost::thread* mSendToDccThread;
	boost::thread* mReceiveFromDccThread;
};
