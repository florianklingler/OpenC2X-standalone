#ifndef LDM_H_
#define LDM_H_

#include <boost/thread.hpp>
#include <utility/CommunicationReceiver.h>
#include <utility/LoggingUtility.h>

class LDM {
public:
	LDM();
	~LDM();
	void init();

	void receiveFromCa();
	void receiveFromDen();

private:
	CommunicationReceiver* mReceiverFromDen;
	CommunicationReceiver* mReceiverFromCa;

	boost::thread* mThreadReceiveFromCa;
	boost::thread* mThreadReceiveFromDen;

	LoggingUtility* mLogger;
};

#endif
