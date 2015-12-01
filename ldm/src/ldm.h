#include <zmq.hpp>
#include <boost/thread.hpp>

class LDM {
public:
	LDM();
	~LDM();
	void init();

  	void receiveLoop();
  	
private:
	zmq::context_t* context;
	zmq::socket_t* subscriber;
	
	boost::thread* receiveThread;
};
