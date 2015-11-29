#include <zmq.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <memory>

using namespace std;

class CAM {
public:
	static boost::shared_ptr<CAM> createCAM();
	virtual ~CAM();
	void init();
	void start();
	void cleanUp();
protected:
	CAM();
private:
	zmq::context_t* ctx;
	zmq::socket_t* mSendSocket; // Send CAM to DCC
	zmq::socket_t* mReceiveSocket; // Receive CAM from DCC
	boost::thread* mSenderThread;
	boost::thread* mReceiverThread;
	void loop();
	void sendLoop();
	void receiveLoop();
};
