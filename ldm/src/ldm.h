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
#include <buffers/build/ldmData.pb.h>
#include <google/protobuf/text_format.h>
#include <string>
#include <ctime>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

struct LdmConfig {
	int mExpNo;	//number of experiment, suffix for DB-name

	void loadConfigXML(const string &filename) {
		boost::property_tree::ptree pt;
		read_xml(filename, pt);

		mExpNo = pt.get("ldm.expNo", 1);
	}
};

class LDM {
public:
	LDM(LdmConfig &config);
	~LDM();
	void init();

	void createTables();
	dataPackage::LdmData gpsSelect(std::string condition);
	dataPackage::LdmData obd2Select(std::string condition);
	dataPackage::LdmData camSelect(std::string condition);
	dataPackage::LdmData denmSelect(std::string condition);
	dataPackage::LdmData dccInfoSelect(std::string condition);
	dataPackage::LdmData camInfoSelect(std::string condition);

	void insert(std::string sqlCommand);
	void insertCam(camPackage::CAM cam);
	void insertDenm(denmPackage::DENM denm);

	std::string readableTime(int64_t nanoTime);
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
	LdmConfig mConfig;

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
