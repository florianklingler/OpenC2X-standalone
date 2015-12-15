#ifndef DCC_H_
#define DCC_H_

#include <boost/thread.hpp>
#include <utility/CommunicationReceiver.h>
#include <utility/CommunicationSender.h>

class DCC {
public:
	DCC();
	~DCC();

	void init();
	void receiveFromCa();
	void receiveFromDen();
	void receiveFromHw();

private:
	CommunicationReceiver* mReceiverFromCa;
	CommunicationReceiver* mReceiverFromDen;
	CommunicationReceiver* mReceiverFromHw;
	CommunicationSender* mSenderToHw;
	CommunicationSender* mSenderToServices;

	boost::thread* mThreadReceiveFromCa;
	boost::thread* mThreadReceiveFromDen;
	boost::thread* mThreadReceiveFromHw;
};
#endif
