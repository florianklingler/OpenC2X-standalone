#ifndef DENSERVICE_H_
#define DENSERVICE_H_

#include <boost/thread.hpp>
#include <utility/CommunicationReceiver.h>
#include <utility/CommunicationSender.h>
#include <utility/LoggingUtility.h>
#include <buffers/build/data.pb.h>
#include <buffers/build/denm.pb.h>
#include <buffers/build/gps.pb.h>
#include <mutex>

class DenService {
public:
	DenService();
	~DenService();

	void init();
	void receive();
	void logDelay(string byteMessage);
	void triggerDenm();
	void send();
	denmPackage::DENM generateDenm();
	dataPackage::DATA generateData(denmPackage::DENM denm);
	void receiveGpsData();

private:
	void microSleep(double us_sleep); // in us

	CommunicationReceiver* mReceiverFromDcc;
	CommunicationSender* mSenderToDcc;
	CommunicationSender* mSenderToLdm;

	CommunicationReceiver* mReceiverGps;

	boost::thread* mThreadReceive;
	boost::thread* mThreadGpsDataReceive;
	boost::thread* mThreadSend;

	LoggingUtility* mLogger;

	long mIdCounter;

	gpsPackage::GPS mLatestGps;
	mutex mMutexLatestGps;
};

#endif
