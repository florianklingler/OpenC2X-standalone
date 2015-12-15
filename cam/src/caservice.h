#ifndef CASERVICE_H_
#define CASERVICE_H_

#include <string>
#include <boost/thread.hpp>
#include <utility/CommunicationReceiver.h>
#include <utility/CommunicationSender.h>

using namespace std;

class CaService {
public:
	CaService();
	~CaService();

	void init();
	void receive();
	void send();
	string generateCam();

private:
	CommunicationReceiver* mReceiverFromDcc;
	CommunicationSender* mSenderToDcc;
	CommunicationSender* mSenderToLdm;

	boost::thread* mThreadReceive;
	boost::thread* mThreadSend;

	long mIdCounter;
};
#endif
