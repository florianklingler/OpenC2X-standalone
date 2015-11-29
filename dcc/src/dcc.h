#include "zmq.hpp"
#include "zhelpers.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

using namespace std;

class DCC {
public:
	static boost::shared_ptr<DCC> createDCC();
	virtual ~DCC();
	void init();
	void start();
	void cleanUp();
protected:
	DCC();

private:
	zmq::context_t* ctx;
	zmq::socket_t* mSendSocket; // Forward CAM received from other cars to CAM service
	zmq::socket_t* mReceiveSocket; // Receive CAM from CAM service
	boost::thread* mSenderThread;
	boost::thread* mReceiverThread;

	void loop();
	void sendLoop();
	void receiveLoop();
};
