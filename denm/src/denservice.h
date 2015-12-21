#ifndef DENSERVICE_H_
#define DENSERVICE_H_

#include <boost/thread.hpp>
#include <utility/CommunicationReceiver.h>
#include <utility/CommunicationSender.h>
#include <utility/LoggingUtility.h>
#include <buffers/build/wrapper.pb.h>
#include <buffers/build/denm.pb.h>

class DenService {
public:
	DenService();
	~DenService();

	void init();
	void receive();
	void logDelay(string byteMessage);
	void send();
	denmPackage::DENM generateDenm();
	wrapperPackage::WRAPPER generateWrapper(denmPackage::DENM denm);

private:
	CommunicationReceiver* mReceiverFromDcc;
	CommunicationSender* mSenderToDcc;
	CommunicationSender* mSenderToLdm;

	boost::thread* mThreadReceive;
	boost::thread* mThreadSend;

	LoggingUtility* mLogger;

	long mIdCounter;
};

#endif
