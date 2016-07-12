#ifndef LDM_H_
#define LDM_H_

/**
 * @addtogroup ldm
 * @{
 */

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
#include <mutex>

/**
 * Responsible for Data holding and maintainance.
 *
 * Writes into a SQL database. Caches newest incoming data to answer
 * requests because sql querys are to slow.
 *
 * @nonStandard deleting of old data. for debugging add "deleted" flag and automatically "delete" entries that are eg. too old, too far away; only return non-delted entries
 */
class LDM {
public:
	LDM();
	~LDM();
	void init();

	void createTables();

	dataPackage::LdmData gpsSelect(std::string condition);	//TODO: only return latest GPS? useful?
	dataPackage::LdmData obd2Select(std::string condition);
	dataPackage::LdmData camSelect(std::string condition);
	dataPackage::LdmData denmSelect(std::string condition);
	dataPackage::LdmData dccInfoSelect(std::string condition);
	dataPackage::LdmData camInfoSelect(std::string condition);

	void insert(std::string sqlCommand);
	void insertCam(camPackage::CAM cam);
	void insertDenm(denmPackage::DENM denm);

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

	std::mutex mCamMutex;
	std::mutex mCamInfoMutex;
	std::mutex mDccInfoMutex;
	std::mutex mGpsMutex;
	std::mutex mObd2Mutex;
	std::mutex mDenmMutex;

	sqlite3* mDb;

	std::map<std::string,camPackage::CAM> camCache;
	infoPackage::CamInfo camInfoCache;
	std::map<std::string,infoPackage::DccInfo>  dccInfoCache;
	std::map<std::string,denmPackage::DENM> denmCache;
};

/** @} */ //end group
#endif
