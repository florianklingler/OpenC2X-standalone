#ifndef CASERVICE_H_
#define CASERVICE_H_

#include <boost/thread.hpp>
#include <utility/CommunicationReceiver.h>
#include <utility/CommunicationSender.h>

class CaService {
public:
	CaService();
	~CaService();

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
