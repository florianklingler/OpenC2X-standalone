#include <utility/ICommunication.h>
#include "CommunicationSender.h"

using namespace std;

class Communication : public CommunicationSender {
public:
	Communication(string portIn, string portOut, string envelope, ICommunication* communicator);
	~Communication();
	void run();

private:
	zmq::socket_t* mSubscriber;

	ICommunication* mCommunicator;

	string receive();
};
