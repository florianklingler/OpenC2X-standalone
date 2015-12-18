#ifndef CASERVICE_H_
#define CASERVICE_H_

#include <boost/thread.hpp>
#include <utility/CommunicationReceiver.h>
#include <utility/CommunicationSender.h>
#include <buffers/build/wrapper.pb.h>
#include <buffers/build/cam.pb.h>
#include <boost/asio.hpp>

class CaService {
public:
	CaService();
	~CaService();

	void init();
	void receive();
	void logDelay(string byteMessage);
	void send();
	void triggerCam(const boost::system::error_code &ec);
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

	boost::asio::io_service mIoService;
	boost::asio::deadline_timer* mTimer;

	long mIdCounter;

	double mCamTriggerInterval;
};

#endif
