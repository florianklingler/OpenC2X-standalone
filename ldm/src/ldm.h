#ifndef LDM_H_
#define LDM_H_

#include <boost/thread.hpp>
#include <utility/CommunicationReceiver.h>
#include <utility/CommunicationServer.h>
#include <utility/LoggingUtility.h>
#include <sqlite3.h>
#include <buffers/build/cam.pb.h>
#include <buffers/build/denm.pb.h>
#include <buffers/build/gps.pb.h>
#include <buffers/build/obd2.pb.h>
#include <buffers/build/dccInfo.pb.h>
#include <buffers/build/camInfo.pb.h>
#include <google/protobuf/text_format.h>
#include <string>
#include <ctime>

class LDM {
public:
	LDM();
	~LDM();
	void init();

	//TODO: auto generate table if not existing

	list<gpsPackage::GPS> gpsSelect(string condition);
	list<obd2Package::OBD2> obd2Select(string condition);
	list<camPackage::CAM> camSelect(string condition);
	list<denmPackage::DENM> denmSelect(string condition);
	//TODO: select dcc/camInfo
	void insert(string sqlCommand);
	void insertCam(camPackage::CAM cam);
	void insertDenm(denmPackage::DENM denm);

	string readableTime(int64_t nanoTime);
	void printGps(gpsPackage::GPS gps);
	void printObd2(obd2Package::OBD2 obd2);
	void printCam(camPackage::CAM cam);
	void printDenm(denmPackage::DENM denm);

	void receiveFromCa();
	void receiveFromDen();
	void receiveRequest();
	void receiveDccInfo();
	void receiveCamInfo();

private:
	CommunicationReceiver* mReceiverFromDen;
	CommunicationReceiver* mReceiverFromCa;
	CommunicationReceiver* mReceiverDccInfo;
	CommunicationReceiver* mReceiverCamInfo;
	CommunicationServer* mServer;

	boost::thread* mThreadReceiveFromCa;
	boost::thread* mThreadReceiveFromDen;
	boost::thread* mThreadReceiveDccInfo;
	boost::thread* mThreadReceiveCamInfo;
	boost::thread* mThreadServer;

	LoggingUtility* mLogger;

	sqlite3* mDb;
};

#endif
