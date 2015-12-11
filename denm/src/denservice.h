#ifndef DENSERVICE_H_
#define DENSERVICE_H_

#include <boost/thread.hpp>
#include <utility/CommunicationReceiver.h>
#include <utility/CommunicationSender.h>

class DenService {
public:
	DenService();
	~DenService();

	void init();
	void receive();
	void send();

private:
	CommunicationReceiver* mReceiverFromDcc;
	CommunicationSender* mSenderToDcc;
	CommunicationSender* mSenderToLdm;

	boost::thread* mThreadReceive;
	boost::thread* mThreadSend;
};

#endif
