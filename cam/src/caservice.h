#ifndef CASERVICE_H_
#define CASERVICE_H_

#include <boost/thread.hpp>
#include <utility/CommunicationReceiver.h>
#include <utility/CommunicationSender.h>
#include <buffers/build/wrapper.pb.h>
#include <buffers/build/cam.pb.h>

class CaService {
public:
	CaService();
	~CaService();

	void init();
	void receive();
	void logDelay(string byteMessage);
	void send();
	camPackage::CAM generateCam();
	wrapperPackage::WRAPPER generateWrapper(camPackage::CAM cam);

private:

	void microSleep(double us_sleep); // in us

	CommunicationReceiver* mReceiverFromDcc;
	CommunicationSender* mSenderToDcc;
	CommunicationSender* mSenderToLdm;

	boost::thread* mThreadReceive;
	boost::thread* mThreadSend;

	LoggingUtility* mLogger;

	long mIdCounter;
};

#endif
