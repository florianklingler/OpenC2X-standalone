#ifndef DENSERVICE_H_
#define DENSERVICE_H_

#include <boost/thread.hpp>
#include <utility/CommunicationReceiver.h>
#include <utility/CommunicationSender.h>
#include <utility/LoggingUtility.h>
#include <buffers/build/data.pb.h>
#include <buffers/build/denm.pb.h>
#include <buffers/build/gps.pb.h>
#include <buffers/build/obd2.pb.h>
#include <buffers/build/trigger.pb.h>
#include <mutex>

class DenService {
public:
	DenService();
	~DenService();

	void init();
	void receive();
	void logDelay(std::string byteMessage);
	void triggerPeriodicDenm();
	void triggerAppDenm();
	void send(triggerPackage::TRIGGER trigger);
	denmPackage::DENM generateDenm(triggerPackage::TRIGGER trigger);
	dataPackage::DATA generateData(denmPackage::DENM denm);
	void receiveGpsData();
	void receiveObd2Data();

private:
	void microSleep(double us_sleep); // in us

	CommunicationReceiver* mReceiverFromApp;
	CommunicationReceiver* mReceiverFromDcc;
	CommunicationSender* mSenderToDcc;
	CommunicationSender* mSenderToLdm;

	CommunicationReceiver* mReceiverGps;
	CommunicationReceiver* mReceiverObd2;

	boost::thread* mThreadReceive;
	boost::thread* mThreadGpsDataReceive;
	boost::thread* mThreadObd2DataReceive;
	boost::thread* mThreadAppTrigger;
	boost::thread* mThreadSend;

	LoggingUtility* mLogger;

	long mIdCounter;

	gpsPackage::GPS mLatestGps;
	std::mutex mMutexLatestGps;

	obd2Package::OBD2 mLatestObd2;
	std::mutex mMutexLatestObd2;
};

#endif
