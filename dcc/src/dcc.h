#ifndef DCC_H_
#define DCC_H_

#include "SendToHardwareViaIP.h"
#include <boost/thread.hpp>
#include <utility/CommunicationReceiver.h>
#include <utility/CommunicationSender.h>

#include <mutex>

class RecieveFromHarwareViaIP;

class DCC {
public:
	DCC();
	~DCC();

	void init();
	void receiveFromCa();
	void receiveFromDen();
	void receiveFromHw(string msg);

private:
	CommunicationReceiver* mReceiverFromCa;
	CommunicationReceiver* mReceiverFromDen;
	CommunicationSender* mSenderToServices;

	boost::thread* mThreadReceiveFromCa;
	boost::thread* mThreadReceiveFromDen;

	SendToHardwareViaIP* mSenderToHw;
	RecieveFromHarwareViaIP* mRecieveFromHw;
};
#endif
