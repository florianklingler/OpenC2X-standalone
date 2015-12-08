#include <zmq.hpp>
#include <string>
#include <boost/thread.hpp>
#include <utility/Communication.h>

using namespace std;

class DENM {
public:
	DENM();
	~DENM();
	void init();
	void loop();
	void receiveFromDccLoop();
	void sendToDccLoop();
	void sendToLDM();
	
	string process(string msg){
		return msg;
	};

	
private:
	Communication* mCommunication;
	
	boost::thread* mSendToDccThread;
	boost::thread* mReceiveFromDccThread;
};
