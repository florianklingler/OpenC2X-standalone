#include <zmq.hpp>
#include <string>

using namespace std;

class Communication
{
public:
	Communication(int portIn, int portOut, string envelope, string (*process)(string message));
	~Communication();
	void run();
	void send(string msg);



private:
	zmq::context_t* mContext;
	zmq::socket_t* mSubscriber;
	zmq::socket_t* mPublisher;

	string mEnvelope;
	int mPortOut;
	string (*mProcess) (string message);

	string receive();
};
