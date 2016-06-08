#ifndef CASERVICE_H_
#define CASERVICE_H_

#include <boost/thread.hpp>
#include <config/config.h>
#include <utility/CommunicationReceiver.h>
#include <utility/CommunicationSender.h>
#include <buffers/build/data.pb.h>
#include <buffers/build/cam.pb.h>
#include <buffers/build/gps.pb.h>
#include <buffers/build/obd2.pb.h>
#include <buffers/build/camInfo.pb.h>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <mutex>

struct CaServiceConfig {
	bool mGenerateMsgs;
	int mExpirationTime;
	int mMaxGpsAge;
	int mMaxObd2Age;

	void loadConfigXML(const std::string &filename) {
		boost::property_tree::ptree pt;
		read_xml(filename, pt);

		mGenerateMsgs = pt.get("cam.generateMsgs", true);
		mExpirationTime = pt.get("cam.expirationTime", 1);
		mMaxGpsAge = pt.get("cam.maxGpsAge", 10);
		mMaxObd2Age = pt.get("cam.maxObd2Age", 10);
	}
};

class CaService {
public:
	CaService(CaServiceConfig &config);
	~CaService();

	void receive();
	void logDelay(std::string byteMessage);
	void sendCamInfo(std::string triggerReason, double delta);
	void send();
	void triggerCam(const boost::system::error_code &ec);
	camPackage::CAM generateCam();
	dataPackage::DATA generateData(camPackage::CAM cam);
	void receiveGpsData();
	void receiveObd2Data();
	double getHeading(double lat1, double lon1, double lat2, double lon2);
	double getDistance(double lat1, double lon1, double lat2, double lon2);

private:
	GlobalConfig mGlobalConfig;
	CaServiceConfig mConfig;

	CommunicationReceiver* mReceiverFromDcc;
	CommunicationSender* mSenderToDcc;
	CommunicationSender* mSenderToLdm;

	CommunicationReceiver* mReceiverGps;
	CommunicationReceiver* mReceiverObd2;

	boost::thread* mThreadReceive;
	boost::thread* mThreadGpsDataReceive;
	boost::thread* mThreadObd2DataReceive;

	LoggingUtility* mLogger;

	boost::asio::io_service mIoService;
	boost::asio::deadline_timer* mTimer;

	long mIdCounter;
	double mCamTriggerInterval;

	gpsPackage::GPS mLatestGps;
	bool mGpsValid;		//true if GPS was received and it's not too old
	std::mutex mMutexLatestGps;

	obd2Package::OBD2 mLatestObd2;
	bool mObd2Valid;
	std::mutex mMutexLatestObd2;

	camPackage::CAM mLastSentCam;
};

#endif
