#ifndef CASERVICE_H_
#define CASERVICE_H_

#include <boost/thread.hpp>
#include <utility/CommunicationReceiver.h>
#include <utility/CommunicationSender.h>
#include <buffers/build/data.pb.h>
#include <buffers/build/cam.pb.h>
#include <boost/asio.hpp>

struct CaConfig {
	double mCamTriggerInterval;

	void loadConfigXML(const std::string &filename);
};

class CaService {
public:
	CaService(CaConfig &config);
	~CaService();

	void init();
	void receive();
	void logDelay(string byteMessage);
	void send();
	void triggerCam(const boost::system::error_code &ec);
	camPackage::CAM generateCam();
	dataPackage::DATA generateData(camPackage::CAM cam);
	void receiveGpsData();

private:
	CommunicationReceiver* mReceiverFromDcc;
	CommunicationSender* mSenderToDcc;
	CommunicationSender* mSenderToLdm;

	CommunicationReceiver* mReceiverGps;

	boost::thread* mThreadReceive;
	boost::thread* mThreadGpsDataReceive;

	LoggingUtility* mLogger;
	CaConfig mConfig;

	boost::asio::io_service mIoService;
	boost::asio::deadline_timer* mTimer;

	long mIdCounter;
	double mCamTriggerInterval;
};

#endif
