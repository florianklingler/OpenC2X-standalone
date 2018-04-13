// This file is part of OpenC2X.
//
// OpenC2X is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenC2X is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OpenC2X.  If not, see <http://www.gnu.org/licenses/>.
//
// Authors:
// Sven Laux <slaux@mail.uni-paderborn.de>
// Gurjashan Singh Pannu <gurjashan.pannu@ccs-labs.org>
// Stefan Schneider <stefan.schneider@ccs-labs.org>
// Jan Tiemann <janhentie@web.de>


#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include "ldm.h"
#include <unistd.h>
#include <iostream>
#include <common/config/config.h>
#include <map>
#include <common/utility/Utils.h>

using namespace std;

INITIALIZE_EASYLOGGINGPP

LDM::LDM() {
	GlobalConfig config;
	try {
		config.loadConfig(LDM_CONFIG_NAME);
	}
	catch (std::exception &e) {
		cerr << "Error while loading config.xml: " << e.what() << endl;
	}
	ptree pt = load_config_tree();

	mLogger = new LoggingUtility(LDM_CONFIG_NAME, LDM_MODULE_NAME, config.mLogBasePath, config.mExpName, config.mExpNo, pt);

	string moduleName = "Ldm";
	mReceiverFromCa = new CommunicationReceiver("8888", "CAM", *mLogger);
	mReceiverFromDen = new CommunicationReceiver("9999", "DENM", *mLogger);
	mReceiverDccInfo = new CommunicationReceiver("1234", "dccInfo", *mLogger);
	mReceiverCamInfo = new CommunicationReceiver("8888", "camInfo", *mLogger);
	mServer = new CommunicationServer("6789", *mLogger);
	
	std::string db_path = get_openc2x_path(config.mLogBasePath, config.mExpName, config.mExpNo) + "ldm.db";

	//open SQLite database
	if(sqlite3_open_v2((db_path).c_str(), &mDb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL)) {
		mLogger->logError("Cannot open database");
		sqlite3_close(mDb);
	}
	else {
		mLogger->logInfo("Opened database successfully");
		createTables();
	}

	//optimize, eg. don't wait until inserts are written to disk
	char* errmsg = 0;
	if (sqlite3_exec(mDb, "PRAGMA synchronous = OFF", NULL, 0, &errmsg)) {
		string error(errmsg);
		mLogger->logError("SQL error: " + error);
		sqlite3_free(errmsg);
	}
	if (sqlite3_exec(mDb, "PRAGMA default_temp_store = MEMORY", NULL, 0, &errmsg)) {
		string error(errmsg);
		mLogger->logError("SQL error: " + error);
		sqlite3_free(errmsg);
	}
	if (sqlite3_exec(mDb, "PRAGMA journal_mode = MEMORY", NULL, 0, &errmsg)) {
		string error(errmsg);
		mLogger->logError("SQL error: " + error);
		sqlite3_free(errmsg);
	}
}

LDM::~LDM() {
	mThreadReceiveFromCa->join();
	mThreadReceiveFromDen->join();
	mThreadReceiveDccInfo->join();
	mThreadReceiveCamInfo->join();
	mThreadServer->join();
	delete mThreadReceiveFromCa;
	delete mThreadReceiveFromDen;
	delete mThreadReceiveDccInfo;
	delete mThreadReceiveCamInfo;
	delete mThreadServer;

	delete mReceiverFromCa;
	delete mReceiverFromDen;
	delete mReceiverDccInfo;
	delete mReceiverCamInfo;

	sqlite3_close(mDb);
}

void LDM::init() {
	mThreadReceiveFromCa = new boost::thread(&LDM::receiveFromCa, this);
	mThreadReceiveFromDen = new boost::thread(&LDM::receiveFromDen, this);
	mThreadReceiveDccInfo = new boost::thread(&LDM::receiveDccInfo, this);
	mThreadReceiveCamInfo = new boost::thread(&LDM::receiveCamInfo, this);
	mThreadServer = new boost::thread(&LDM::receiveRequest, this);
}


//////////SQLite functions

//creates necessary tables if they don't exist already
void LDM::createTables() {
	char* sqlCommand;
	char* errmsg = 0;

	//create CAM table
	sqlCommand = (char*) "CREATE TABLE IF NOT EXISTS CAM(" \
			"key INTEGER PRIMARY KEY, protocolVersion INTEGER, messageId INTEGER, stationId INTEGER, "\
			"genDeltaTime INTEGER, stationType INTEGER, latitude INTEGER, longitude INTEGER, semiMajorConfidence INTEGER, "\
			"semiMinorConfidence INTEGER, semiMajorOrientation INTEGER, altitude INTEGER, altitudeConfidenc INTEGER, "\
			"type  INTEGER, heading INTEGER, headingConfidence INTEGER, speed INTEGER, speedConfidence INTEGER, driveDirection INTEGER, "\
			"vehicleLength INTEGER, vehicleLengthConfidence INTEGER, vehicleWidth INTEGER, longitudinalAcceleration INTEGER, "\
			"longitudinalAccelerationConfidence INTEGER, curvature INTEGER, curvatureConfidence INTEGER, curvatureCalcMode INTEGER, "\
			"yawRate INTEGER, yawRateConfidence INTEGER, accelerationControl INTEGER, lanePosition INTEGER, steeringWheelAngle INTEGER, "\
			"steeringWheelAngleConfidence INTEGER, lateralAcceleration INTEGER, lateralAccelerationConfidence INTEGER, verticalAcceleration INTEGER, "\
			"verticalAccelerationConfidence INTEGER, performanceClass INTEGER, protectedZoneLatitude INTEGER, protectedZoneLongitude INTEGER, "\
			"cendSrcTollingZoneId INTEGER);";
	if (sqlite3_exec(mDb, sqlCommand, NULL, 0, &errmsg)) {
		string error(errmsg);
		mLogger->logError("SQL error: " + error);
		sqlite3_free(errmsg);
	}

	//create DENM table
	sqlCommand = (char*) "CREATE TABLE IF NOT EXISTS DENM("\
				"key INTEGER PRIMARY KEY, protocolVersion INTEGER, messageId INTEGER, stationId INTEGER, "\
				"sequenceNumber INTEGER, detectionTime INTEGER, referenceTime INTEGER, latitude INTEGER, longitude INTEGER, "\
				"semiMajorConfidence INTEGER, semiMinorConfidence INTEGER, semiMajorOrientation INTEGER, altitude INTEGER, "\
				"altitudeConfidence INTEGER, validityDuration INTEGER, stationType INTEGER);";
	if (sqlite3_exec(mDb, sqlCommand, NULL, 0, &errmsg)) {
		string error(errmsg);
		mLogger->logError("SQL error: " + error);
		sqlite3_free(errmsg);
	}

	//create GPS table
	sqlCommand = (char*) "CREATE TABLE IF NOT EXISTS GPS(" \
				"key INTEGER PRIMARY KEY, latitude DOUBLE, longitude DOUBLE, altitude DOUBLE, epx DOUBLE, epy DOUBLE, time INTEGER, online INTEGER, satellites INTEGER);";
	if (sqlite3_exec(mDb, sqlCommand, NULL, 0, &errmsg)) {
		string error(errmsg);
		mLogger->logError("SQL error: " + error);
		sqlite3_free(errmsg);
	}

	//create OBD2 table
	sqlCommand = (char*) "CREATE TABLE IF NOT EXISTS OBD2(" \
				"key INTEGER PRIMARY KEY, time INTEGER, speed DOUBLE, rpm INTEGER);";
	if (sqlite3_exec(mDb, sqlCommand, NULL, 0, &errmsg)) {
		string error(errmsg);
		mLogger->logError("SQL error: " + error);
		sqlite3_free(errmsg);
	}

	//create DccInfo table
	sqlCommand = (char*) "CREATE TABLE IF NOT EXISTS DccInfo(" \
				"key INTEGER PRIMARY KEY, time INTEGER, channelLoad DOUBLE, state TEXT, AC TEXT, availableTokens INTEGER, queuedPackets INTEGER, dccMechanism INTEGER, txPower DOUBLE, tokenInterval DOUBLE, datarate DOUBLE, carrierSense DOUBLE, flushReqPackets INTEGER, flushNotReqPackets INTEGER);";
	if (sqlite3_exec(mDb, sqlCommand, NULL, 0, &errmsg)) {
		string error(errmsg);
		mLogger->logError("SQL error: " + error);
		sqlite3_free(errmsg);
	}

	//create CamInfo table
	sqlCommand = (char*) "CREATE TABLE IF NOT EXISTS CamInfo(" \
				"key INTEGER PRIMARY KEY, time INTEGER, triggerReason TEXT, delta DOUBLE);";
	if (sqlite3_exec(mDb, sqlCommand, NULL, 0, &errmsg)) {
		string error(errmsg);
		mLogger->logError("SQL error: " + error);
		sqlite3_free(errmsg);
	}
}


//executes SELECT with specified condition (eg. WHERE) on GPS table and returns result
dataPackage::LdmData LDM::gpsSelect(string condition) {
	dataPackage::LdmData result;
	result.set_type(dataPackage::LdmData_Type_GPS);
	sqlite3_stmt *stmt;
	string sqlCommand = "SELECT * from GPS " + condition;
	char* errmsg = 0;

	mGpsMutex.lock();
	if (sqlite3_prepare_v2(mDb, sqlCommand.c_str(), -1, &stmt, NULL) == SQLITE_OK) {
		while (sqlite3_step(stmt) == SQLITE_ROW) {		//iterate over result rows
			gpsPackage::GPS gps;

			//set attributes retrieved from result columns
			gps.set_latitude(sqlite3_column_double(stmt, 1));
			gps.set_longitude(sqlite3_column_double(stmt, 2));
			gps.set_altitude(sqlite3_column_double(stmt, 3));
			gps.set_epx(sqlite3_column_double(stmt, 4));
			gps.set_epy(sqlite3_column_double(stmt, 5));
			gps.set_time(sqlite3_column_int64(stmt, 6));
			gps.set_online(sqlite3_column_int64(stmt, 7));
			gps.set_satellites(sqlite3_column_int(stmt, 8));

			//add to result
			string serializedGps;
			gps.SerializeToString(&serializedGps);
			result.add_data(serializedGps);
		}
		sqlite3_finalize(stmt);
	}
	else {
		string error(errmsg);
		mLogger->logError("SQL error: " + error);
		sqlite3_free(errmsg);
	}
	mGpsMutex.unlock();
	return result;
}

//executes specified SELECT with specified condition (eg. WHERE) on OBD2 table and returns result
dataPackage::LdmData LDM::obd2Select(string condition) {
	dataPackage::LdmData result;
	result.set_type(dataPackage::LdmData_Type_OBD2);
	sqlite3_stmt *stmt;
	string sqlCommand = "SELECT * from OBD2 " + condition;
	char* errmsg = 0;
	mObd2Mutex.lock();
	if (sqlite3_prepare_v2(mDb, sqlCommand.c_str(), -1, &stmt, NULL) == SQLITE_OK) {
		while (sqlite3_step(stmt) == SQLITE_ROW) {		//iterate over result rows
			obd2Package::OBD2 obd2;

			//set attributes retrieved from result columns
			obd2.set_time(sqlite3_column_int64(stmt, 1));
			obd2.set_speed(sqlite3_column_double(stmt, 2));
			obd2.set_rpm(sqlite3_column_int(stmt, 3));

			//add to result
			string serializedObd2;
			obd2.SerializeToString(&serializedObd2);
			result.add_data(serializedObd2);
		}
		sqlite3_finalize(stmt);
	}
	else {
		string error(errmsg);
		mLogger->logError("SQL error: " + error);
		sqlite3_free(errmsg);
	}
	mObd2Mutex.unlock();
	return result;
}

//executes specified SELECT with specified condition (eg. WHERE) on CAM table and returns result
dataPackage::LdmData LDM::camSelect(string condition) {
	dataPackage::LdmData result;
	result.set_type(dataPackage::LdmData_Type_CAM);

	//initalise cache with content from db
	if (camCache.empty()){
		sqlite3_stmt *stmt;
		//sql from http://stackoverflow.com/questions/1313120/retrieving-the-last-record-in-each-group; have to use createTime instead of id because id isn't necessarily unique and growing
		string sqlCommand = "SELECT cam1.* from CAM cam1 LEFT JOIN CAM cam2 ON (cam1.stationId = cam2.stationId AND cam1.key < cam2.key) WHERE cam2.key IS NULL";

		char* errmsg = 0;
		mCamMutex.lock();
		if (sqlite3_prepare_v2(mDb, sqlCommand.c_str(), -1, &stmt, NULL) == SQLITE_OK) {
			while (sqlite3_step(stmt) == SQLITE_ROW) {		//iterate over result rows
				camPackage::CAM cam;
				// header
				its::ItsPduHeader* header = new its::ItsPduHeader;
				header->set_messageid(sqlite3_column_int64(stmt, 1));
				header->set_protocolversion(sqlite3_column_int64(stmt, 2));
				header->set_stationid(sqlite3_column_int64(stmt, 3));
				cam.set_allocated_header(header);
				// coop awareness
				its::CoopAwareness* coop = new its::CoopAwareness;
				coop->set_gendeltatime(sqlite3_column_int64(stmt, 4));
				its::CamParameters* params = new its::CamParameters;

				// basic container
				its::BasicContainer* basicContainer = new its::BasicContainer;

				basicContainer->set_stationtype(sqlite3_column_int64(stmt, 5));
				basicContainer->set_latitude(sqlite3_column_int64(stmt, 6));
				basicContainer->set_longitude(sqlite3_column_int64(stmt, 7));
				basicContainer->set_altitude(sqlite3_column_int64(stmt, 11));
				basicContainer->set_altitudeconfidence(sqlite3_column_int64(stmt, 12));
				basicContainer->set_semimajorconfidence(sqlite3_column_int64(stmt, 8));
				basicContainer->set_semiminorconfidence(sqlite3_column_int64(stmt, 9));
				basicContainer->set_semimajororientation(sqlite3_column_int64(stmt, 10));
				params->set_allocated_basiccontainer(basicContainer);

				// high frequency container
				its::HighFreqContainer* highFreqContainer = new its::HighFreqContainer;
				its::BasicVehicleHighFreqContainer* basicHighFreqContainer = 0;
				its::RsuHighFreqContainer* rsuHighFreqContainer = 0;

				switch (sqlite3_column_int64(stmt, 13)) { // based upon type
					case its::HighFreqContainer_Type_BASIC_HIGH_FREQ_CONTAINER:
						highFreqContainer->set_type(its::HighFreqContainer_Type_BASIC_HIGH_FREQ_CONTAINER);
						basicHighFreqContainer = new its::BasicVehicleHighFreqContainer();
						basicHighFreqContainer->set_heading(sqlite3_column_int64(stmt, 14));
						basicHighFreqContainer->set_headingconfidence(sqlite3_column_int64(stmt, 15));
						basicHighFreqContainer->set_speed(sqlite3_column_int64(stmt, 16));
						basicHighFreqContainer->set_speedconfidence(sqlite3_column_int64(stmt, 17));
						basicHighFreqContainer->set_drivedirection(sqlite3_column_int64(stmt, 18));
						basicHighFreqContainer->set_vehiclelength(sqlite3_column_int64(stmt, 19));
						basicHighFreqContainer->set_vehiclelengthconfidence(sqlite3_column_int64(stmt, 20));
						basicHighFreqContainer->set_vehiclewidth(sqlite3_column_int64(stmt, 21));
						basicHighFreqContainer->set_longitudinalacceleration(sqlite3_column_int64(stmt, 22));
						basicHighFreqContainer->set_longitudinalaccelerationconfidence(sqlite3_column_int64(stmt, 23));
						basicHighFreqContainer->set_curvature(sqlite3_column_int64(stmt, 24));
						basicHighFreqContainer->set_curvatureconfidence(sqlite3_column_int64(stmt, 25));
						basicHighFreqContainer->set_curvaturecalcmode(sqlite3_column_int64(stmt, 26));
						basicHighFreqContainer->set_yawrate(sqlite3_column_int64(stmt, 27));
						basicHighFreqContainer->set_yawrateconfidence(sqlite3_column_int64(stmt, 28));

						// optional fields
						//basicHighFreqContainer->set_accelerationcontrol();
						//basicHighFreqContainer->set_laneposition();
						//basicHighFreqContainer->set_steeringwheelangle();
						//basicHighFreqContainer->set_steeringwheelangleconfidence();
						//basicHighFreqContainer->set_lateralacceleration();
						//basicHighFreqContainer->set_lateralaccelerationconfidence();
						//basicHighFreqContainer->set_verticalacceleration();
						//basicHighFreqContainer->set_verticalaccelerationconfidence();
						//basicHighFreqContainer->set_performanceclass();
						//basicHighFreqContainer->set_protectedzonelatitude();
						//basicHighFreqContainer->set_has_protectedzonelongitude();
						//basicHighFreqContainer->set_cendsrctollingzoneid();

						highFreqContainer->set_allocated_basicvehiclehighfreqcontainer(basicHighFreqContainer);
						break;

					case its::HighFreqContainer_Type_RSU_HIGH_FREQ_CONTAINER:
						highFreqContainer->set_type(its::HighFreqContainer_Type_RSU_HIGH_FREQ_CONTAINER);

						rsuHighFreqContainer = new its::RsuHighFreqContainer();
						// optional fields
						//rsuHighFreqContainer->

						highFreqContainer->set_allocated_rsuhighfreqcontainer(rsuHighFreqContainer);
						break;

					default:
						break;
				}
				//add result
				camCache[to_string(cam.header().stationid())]=cam;
			}
			sqlite3_finalize(stmt);
		}
		else {
			string error(errmsg);
			mLogger->logError("SQL error: " + error);
			sqlite3_free(errmsg);
		}
		mCamMutex.unlock();
	}

	for (auto entry : camCache){
		string serializedCam;
		entry.second.SerializeToString(&serializedCam);
		result.add_data(serializedCam);
	}
	return result;
}

//executes specified SELECT with specified condition (eg. WHERE) on DENM table and returns result
dataPackage::LdmData LDM::denmSelect(string condition) {
	dataPackage::LdmData result;
	result.set_type(dataPackage::LdmData_Type_DENM);

	//initalize cache from db
	if (denmCache.empty()){
		sqlite3_stmt *stmt;
		//only get the latest DENM for each stationId
		string sqlCommand = "SELECT denm1.* from DENM denm1 LEFT JOIN DENM denm2 ON (denm1.stationId = denm2.stationId AND denm1.key < denm2.key) WHERE denm2.key IS NULL";
		char* errmsg = 0;
		mDenmMutex.lock();
		if (sqlite3_prepare_v2(mDb, sqlCommand.c_str(), -1, &stmt, NULL) == SQLITE_OK) {
			while (sqlite3_step(stmt) == SQLITE_ROW) {		//iterate over result rows
				denmPackage::DENM denm;

				// header
				its::ItsPduHeader* header = new its::ItsPduHeader;
				header->set_protocolversion(sqlite3_column_int64(stmt, 1));
				header->set_messageid(sqlite3_column_int64(stmt, 2));
				header->set_stationid(sqlite3_column_int64(stmt, 3));
				denm.set_allocated_header(header);

				// DENM containers
				its::DENMessage* denmMsg = new its::DENMessage;
				its::DENMManagementContainer* mgtCtr = new its::DENMManagementContainer;
				mgtCtr->set_stationid(sqlite3_column_int64(stmt, 3));
				mgtCtr->set_sequencenumber(sqlite3_column_int64(stmt, 4));
				//mgtCtr->set_detectiontime(sqlite3_column_int64(stmt, 5));
				//mgtCtr->set_referencetime(sqlite3_column_int64(stmt, 6));
				mgtCtr->set_latitude(sqlite3_column_int64(stmt, 7));
				mgtCtr->set_longitude(sqlite3_column_int64(stmt, 8));
				mgtCtr->set_semimajorconfidence(sqlite3_column_int64(stmt, 9));
				mgtCtr->set_semiminorconfidence(sqlite3_column_int64(stmt, 10));
				mgtCtr->set_semimajororientation(sqlite3_column_int64(stmt, 11));
				mgtCtr->set_altitude(sqlite3_column_int64(stmt, 12));
				mgtCtr->set_altitudeconfidence(sqlite3_column_int64(stmt, 13));
				//mgtCtr->set_validityduration(sqlite3_column_int64(stmt, 14));
				mgtCtr->set_stationtype(sqlite3_column_int64(stmt, 15));
				denmMsg->set_allocated_managementcontainer(mgtCtr);
				denm.set_allocated_msg(denmMsg);

				//add result
				denmCache[to_string(denm.header().stationid())]=denm;
			}
			sqlite3_finalize(stmt);
		}
		else {
			string error(errmsg);
			mLogger->logError("SQL error: " + error);
			sqlite3_free(errmsg);
		}
		mDenmMutex.unlock();
	}
	for (auto entry : denmCache){
		string serializedDenm;
		entry.second.SerializeToString(&serializedDenm);
		result.add_data(serializedDenm);
	}
	return result;
}

//executes specified SELECT with specified condition (eg. WHERE) on dccInfo table and returns result
dataPackage::LdmData LDM::dccInfoSelect(string condition) {
	dataPackage::LdmData result;
	result.set_type(dataPackage::LdmData_Type_dccInfo);

	//init cache from db
	if (dccInfoCache.empty()){
		sqlite3_stmt *stmt;
		string sqlCommand = "SELECT dccInfo1.* from DccInfo dccInfo1 LEFT JOIN DccInfo dccInfo2 ON (dccInfo1.AC = dccInfo2.AC AND dccInfo1.time < dccInfo2.time) WHERE dccInfo2.time IS NULL";
		char* errmsg = 0;
		mDccInfoMutex.lock();
		if (sqlite3_prepare_v2(mDb, sqlCommand.c_str(), -1, &stmt, NULL) == SQLITE_OK) {
			while (sqlite3_step(stmt) == SQLITE_ROW) {		//iterate over result rows
				infoPackage::DccInfo dccInfo;

				//set attributes retrieved from result columns
				dccInfo.set_time(sqlite3_column_int64(stmt, 1));
				dccInfo.set_channelload(sqlite3_column_double(stmt, 2));
				dccInfo.set_state(string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3))));
				dccInfo.set_accesscategory(string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4))));
				dccInfo.set_availabletokens(sqlite3_column_int(stmt, 5));
				dccInfo.set_queuedpackets(sqlite3_column_int(stmt, 6));
				dccInfo.set_dccmechanism(sqlite3_column_int(stmt, 7));
				dccInfo.set_txpower(sqlite3_column_double(stmt, 8));
				dccInfo.set_tokeninterval(sqlite3_column_double(stmt, 9));
				dccInfo.set_datarate(sqlite3_column_double(stmt, 10));
				dccInfo.set_carriersense(sqlite3_column_double(stmt, 11));
				dccInfo.set_flushreqpackets(sqlite3_column_int(stmt, 12));
				dccInfo.set_flushnotreqpackets(sqlite3_column_int(stmt, 13));

				dccInfoCache[dccInfo.accesscategory()]=dccInfo;
			}
			sqlite3_finalize(stmt);
		}
		else {
			string error(errmsg);
			mLogger->logError("SQL error: " + error);
			sqlite3_free(errmsg);
		}
		mDccInfoMutex.unlock();
	}
	for (auto entry : dccInfoCache){
		string serializedDccInfo;
		entry.second.SerializeToString(&serializedDccInfo);
		result.add_data(serializedDccInfo);
	}
	return result;
}

//executes specified SELECT with specified condition (eg. WHERE) on camInfo table and returns result
dataPackage::LdmData LDM::camInfoSelect(string condition) {
	dataPackage::LdmData result;
	result.set_type(dataPackage::LdmData_Type_camInfo);

	//Initialize cache from db
	if(!camInfoCache.has_time()){

		sqlite3_stmt *stmt;
		string sqlCommand = "SELECT * from CamInfo " + condition;
		char* errmsg = 0;
		mCamInfoMutex.lock();
		if (sqlite3_prepare_v2(mDb, sqlCommand.c_str(), -1, &stmt, NULL) == SQLITE_OK) {
			while (sqlite3_step(stmt) == SQLITE_ROW) {		//iterate over result rows

				//set attributes retrieved from result columns
				camInfoCache.set_time(sqlite3_column_int64(stmt, 1));
				camInfoCache.set_triggerreason(string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))));
				camInfoCache.set_delta(sqlite3_column_double(stmt, 3));
			}
			sqlite3_finalize(stmt);
		}
		else {
			string error(errmsg);
			mLogger->logError("SQL error: " + error);
			sqlite3_free(errmsg);
		}
		mCamInfoMutex.unlock();
	}

	//add to result
	string serializedCamInfo;
	camInfoCache.SerializeToString(&serializedCamInfo);
	result.add_data(serializedCamInfo);

	return result;
}

//executes specified insert
void LDM::insert(string sqlCommand) {
	char* errmsg = 0;
	if (sqlite3_exec(mDb, sqlCommand.c_str(), NULL, 0, &errmsg) != SQLITE_OK) {
		string error(errmsg);
		mLogger->logError("SQL error: " + error);
		sqlite3_free(errmsg);
	}
}

//inserts CAM into DB
void LDM::insertCam(camPackage::CAM cam) {
	stringstream sSql;
	sSql << setprecision(15);
	// header
	int64_t protocolversion = cam.header().protocolversion();
	int64_t messageid = cam.header().messageid();
	int64_t stationid = cam.header().stationid();
	// coop awareness
	int64_t gendeltatime = cam.coop().gendeltatime();
	// basic container
	int64_t stationtype = cam.coop().camparameters().basiccontainer().stationtype();
	int64_t latitude = cam.coop().camparameters().basiccontainer().latitude();
	int64_t longitude = cam.coop().camparameters().basiccontainer().longitude();
	int64_t semimajorconfidence = cam.coop().camparameters().basiccontainer().semimajorconfidence();
	int64_t semiminorconfidence = cam.coop().camparameters().basiccontainer().semiminorconfidence();
	int64_t semimajororientation = cam.coop().camparameters().basiccontainer().semimajororientation();
	int64_t altitude = cam.coop().camparameters().basiccontainer().altitude();
	int64_t altitudeconfidence = cam.coop().camparameters().basiccontainer().altitudeconfidence();

	// high frequency container
	int64_t type = cam.coop().camparameters().highfreqcontainer().type();
	int64_t heading = cam.coop().camparameters().highfreqcontainer().basicvehiclehighfreqcontainer().heading();
	int64_t headingconfidence = cam.coop().camparameters().highfreqcontainer().basicvehiclehighfreqcontainer().headingconfidence();
	int64_t speed = cam.coop().camparameters().highfreqcontainer().basicvehiclehighfreqcontainer().speed();
	int64_t speedconfidence = cam.coop().camparameters().highfreqcontainer().basicvehiclehighfreqcontainer().speedconfidence();
	int64_t drivedirection = cam.coop().camparameters().highfreqcontainer().basicvehiclehighfreqcontainer().drivedirection();
	int64_t vehiclelength = cam.coop().camparameters().highfreqcontainer().basicvehiclehighfreqcontainer().vehiclelength();
	int64_t vehiclelengthconfidence = cam.coop().camparameters().highfreqcontainer().basicvehiclehighfreqcontainer().vehiclelengthconfidence();
	int64_t vehiclewidth = cam.coop().camparameters().highfreqcontainer().basicvehiclehighfreqcontainer().vehiclewidth();
	int64_t longitudinalacceleration = cam.coop().camparameters().highfreqcontainer().basicvehiclehighfreqcontainer().longitudinalacceleration();
	int64_t longitudinalaccelerationconfidence = cam.coop().camparameters().highfreqcontainer().basicvehiclehighfreqcontainer().longitudinalaccelerationconfidence();
	int64_t curvature = cam.coop().camparameters().highfreqcontainer().basicvehiclehighfreqcontainer().curvature();
	int64_t curvatureconfidence = cam.coop().camparameters().highfreqcontainer().basicvehiclehighfreqcontainer().curvatureconfidence();
	int64_t curvaturecalcmode = cam.coop().camparameters().highfreqcontainer().basicvehiclehighfreqcontainer().curvaturecalcmode();
	int64_t yawrate = cam.coop().camparameters().highfreqcontainer().basicvehiclehighfreqcontainer().yawrate();
	int64_t yawrateconfidence = cam.coop().camparameters().highfreqcontainer().basicvehiclehighfreqcontainer().yawrateconfidence();
	int64_t accelerationcontrol = cam.coop().camparameters().highfreqcontainer().basicvehiclehighfreqcontainer().accelerationcontrol();
	int64_t laneposition = cam.coop().camparameters().highfreqcontainer().basicvehiclehighfreqcontainer().laneposition();
	int64_t steeringwheelangle = cam.coop().camparameters().highfreqcontainer().basicvehiclehighfreqcontainer().steeringwheelangle();
	int64_t steeringwheelangleconfidence = cam.coop().camparameters().highfreqcontainer().basicvehiclehighfreqcontainer().steeringwheelangleconfidence();
	int64_t lateralacceleration = cam.coop().camparameters().highfreqcontainer().basicvehiclehighfreqcontainer().lateralacceleration();
	int64_t lateralaccelerationconfidence = cam.coop().camparameters().highfreqcontainer().basicvehiclehighfreqcontainer().lateralaccelerationconfidence();
	int64_t verticalacceleration = cam.coop().camparameters().highfreqcontainer().basicvehiclehighfreqcontainer().verticalacceleration();
	int64_t verticalaccelerationconfidence = cam.coop().camparameters().highfreqcontainer().basicvehiclehighfreqcontainer().verticalaccelerationconfidence();
	int64_t performanceclass = cam.coop().camparameters().highfreqcontainer().basicvehiclehighfreqcontainer().performanceclass();
	int64_t protectedzonelatitude = cam.coop().camparameters().highfreqcontainer().basicvehiclehighfreqcontainer().protectedzonelatitude();
	int64_t protectedzonelongitude = cam.coop().camparameters().highfreqcontainer().basicvehiclehighfreqcontainer().protectedzonelongitude();
	int64_t cendsrctollingzoneid = cam.coop().camparameters().highfreqcontainer().basicvehiclehighfreqcontainer().cendsrctollingzoneid();

	sSql << "INSERT INTO CAM (protocolVersion, messageId, stationId, genDeltaTime, stationType, latitude, longitude, semiMajorConfidence,"\
			"semiMinorConfidence, semiMajorOrientation, altitude, altitudeConfidenc, type, heading, headingConfidence, speed, speedConfidence,"\
			"driveDirection, vehicleLength, vehicleLengthConfidence, vehicleWidth, longitudinalAcceleration, longitudinalAccelerationConfidence,"\
			"curvature, curvatureConfidence, curvatureCalcMode, yawRate, yawRateConfidence, accelerationControl, lanePosition, steeringWheelAngle,"\
			"steeringWheelAngleConfidence, lateralAcceleration, lateralAccelerationConfidence, verticalAcceleration, verticalAccelerationConfidence,"\
			"performanceClass, protectedZoneLatitude, protectedZoneLongitude, cendSrcTollingZoneId) VALUES (" << protocolversion << ", " << messageid << ", "\
			<< stationid << ", " << gendeltatime << ", " << stationtype << ", " << latitude << ", " << longitude << ", " << semimajorconfidence << ", "\
			<< semiminorconfidence << ", " << semimajororientation << ", " << altitude << ", " << altitudeconfidence << ", " << type << ", " << heading << ", "\
			<< headingconfidence << ", " << speed << ", " << speedconfidence << ", " << drivedirection << ", " << vehiclelength << ", " << vehiclelengthconfidence << ", "\
			<< vehiclewidth << ", " << longitudinalacceleration << ", " << longitudinalaccelerationconfidence << ", " << curvature << ", " << curvatureconfidence << ", "\
			<< curvaturecalcmode << ", " << yawrate << ", " << yawrateconfidence << ", " << accelerationcontrol << ", " << laneposition << ", " << steeringwheelangle << ", "\
			<< steeringwheelangleconfidence << ", " << lateralacceleration << ", " << lateralaccelerationconfidence << ", " << verticalacceleration << ", "\
			<< verticalaccelerationconfidence << ", " << performanceclass << ", " << protectedzonelatitude << ", " << protectedzonelongitude << ", " << cendsrctollingzoneid << "); ";
	mCamMutex.lock();
	insert(sSql.str());
	mCamMutex.unlock();
}

//inserts DENM into DB
void LDM::insertDenm(denmPackage::DENM denm) {
	stringstream sSql;
	// set decimal precision to 15
	sSql  << setprecision(15);

	// ITS pdu header
	int64_t protocolversion = denm.header().protocolversion();
	int64_t messageid = denm.header().messageid();
	int64_t stationid = denm.header().stationid();

	int64_t sequencenumber = denm.msg().managementcontainer().sequencenumber();
	int64_t detectiontime = denm.msg().managementcontainer().detectiontime();
	int64_t referencetime = denm.msg().managementcontainer().referencetime();
	int64_t latitude = denm.msg().managementcontainer().latitude();
	int64_t longitude = denm.msg().managementcontainer().longitude();
	int64_t semimajorconfidence = denm.msg().managementcontainer().semimajorconfidence();
	int64_t semiminorconfidence = denm.msg().managementcontainer().semiminorconfidence();
	int64_t semimajororientation = denm.msg().managementcontainer().semimajororientation();
	int64_t altitude = denm.msg().managementcontainer().altitude();
	int64_t altitudeconfidence = denm.msg().managementcontainer().altitudeconfidence();
	int64_t validityduration = denm.msg().managementcontainer().validityduration();
	int64_t stationtype = denm.msg().managementcontainer().stationtype();

	sSql << "INSERT INTO DENM (protocolVersion, messageId, stationId, sequenceNumber, detectionTime, referenceTime, latitude, longitude, "\
			"semiMajorConfidence, semiMinorConfidence, semiMajorOrientation, altitude, altitudeConfidence, validityDuration, stationType) VALUES ("\
			<< protocolversion << ", " << messageid << ", " << stationid << ", " << sequencenumber << ", " << detectiontime << ", " << referencetime << ", "\
			<< latitude << ", " << longitude << ", " << semimajorconfidence << ", " << semiminorconfidence << ", " << semimajororientation << ", "\
			<< altitude << ", " << altitudeconfidence << ", " << validityduration << ", " << stationtype << "); ";

	mDenmMutex.lock();
	insert(sSql.str());
	mDenmMutex.unlock();

}

void LDM::printGps(gpsPackage::GPS gps) {
	stringstream stream;
	// set decimal precision to 15
	stream << setprecision(15);

	stream << "GPS - " << Utils::readableTime(gps.time()) << ", lat: " << gps.latitude() << ", long: " << gps.longitude() << ", alt: " << gps.altitude();
	//TODO: include epx, epy, ...?

	mLogger->logInfo(stream.str());
}

void LDM::printObd2(obd2Package::OBD2 obd2) {
	stringstream stream;
	// set decimal precision to 15
	stream  << setprecision(15);

	stream << "OBD2 - " << Utils::readableTime(obd2.time()) << ", speed: " << obd2.speed() << ", rpm: " << obd2.rpm();
	mLogger->logInfo(stream.str());
}

void LDM::printCam(camPackage::CAM cam) {
	stringstream stream;
	// set decimal precision to 15
	stream  << setprecision(15);
	// TODO: implement with new version of CAM
	stream << "Unimplemented printCAM()";
	mLogger->logInfo(stream.str());
}

void LDM::printDenm(denmPackage::DENM denm) {
	stringstream stream;
	// set decimal precision to 15
	stream  << setprecision(15);
	// TODO: implement with new version of DENM
	stream << "Unimplemented printDENM()";
	mLogger->logInfo(stream.str());
}


//////////LDM functions

void LDM::receiveRequest() {
	string envelope, request, reply;
	mLogger->logDebug("waiting for request");
	while(1) {
		pair<string, string> received = mServer->receiveRequest();
		envelope = received.first;	//specifies table
		request = received.second;	//specifies condition

		if (envelope.compare("CAM") == 0) {
			dataPackage::LdmData cams = camSelect(request);
			cams.SerializeToString(&reply);
		}
		else if (envelope.compare("DENM") == 0) {
			dataPackage::LdmData denms = denmSelect(request);
			denms.SerializeToString(&reply);
		}
		else if (envelope.compare("GPS") == 0) {
			dataPackage::LdmData gpss = gpsSelect(request);
			gpss.SerializeToString(&reply);
		}
		else if (envelope.compare("OBD2") == 0) {
			dataPackage::LdmData obd2s = obd2Select(request);
			obd2s.SerializeToString(&reply);
		}
		else if (envelope.compare("dccInfo") == 0) {
			dataPackage::LdmData dccInfos = dccInfoSelect(request);
			dccInfos.SerializeToString(&reply);
		}
		else if (envelope.compare("camInfo") == 0) {
			dataPackage::LdmData camInfos = camInfoSelect(request);
			camInfos.SerializeToString(&reply);
		}
		else {
			reply = request;
		}

		mServer->sendReply(reply);
	}
}

void LDM::receiveFromCa() {
	string serializedCam;	//serialized CAM
	camPackage::CAM cam;

	while (1) {
		pair<string, string> received = mReceiverFromCa->receive();	//receive
		serializedCam = received.second;
		cam.ParseFromString(serializedCam);

		//printCam(cam);
		//ASSUMPTION: received cam is the newer than all cams that were received before.
		//TODO: OPTIMIZATION: use pointers instead of copying cams.
		camCache[to_string(cam.header().stationid())]=cam;
		insertCam(cam);
	}
}

void LDM::receiveFromDen() {
	string serializedDenm;		//serialized DENM
	denmPackage::DENM denm;

	while (1) {
		pair<string, string> received = mReceiverFromDen->receive();//receive
		serializedDenm = received.second;
		denm.ParseFromString(serializedDenm);

		//printDenm(denm);
		//ASSUMPTION: received denm is the newer than all denm that were received before.
		//TODO: OPTIMIZATION: use pointers instead of copying denm.
		denmCache[to_string(denm.header().stationid())] = denm;
		insertDenm(denm);
	}
}

void LDM::receiveDccInfo() {
	string serializedDccInfo;
	infoPackage::DccInfo dccInfo;
	while (1) {
		pair<string, string> received = mReceiverDccInfo->receive();
		serializedDccInfo = received.second;
		dccInfo.ParseFromString(serializedDccInfo);

		stringstream sSql;
		// set decimal precision to 15
		sSql  << setprecision(15);

		sSql << "INSERT INTO DccInfo (time, channelLoad, state, AC, availableTokens, queuedPackets, dccMechanism, txPower, tokenInterval, datarate, carrierSense, flushReqPackets, flushNotReqPackets) ";
		sSql << "VALUES (" << dccInfo.time() << ", " << dccInfo.channelload() << ", '" << dccInfo.state() << "', '" << dccInfo.accesscategory() << "', " << dccInfo.availabletokens() << ", " << dccInfo.queuedpackets() << ", " << dccInfo.dccmechanism() << ", " << dccInfo.txpower() << ", " << dccInfo.tokeninterval() << ", " << dccInfo.datarate() << ", " << dccInfo.carriersense() << ", " << dccInfo.flushreqpackets() << ", " << dccInfo.flushnotreqpackets() << " );";
		mDccInfoMutex.lock();
		insert(sSql.str());
		mDccInfoMutex.unlock();

		//ASSUMPTION: received dccInfo is the newer than all dccInfos that were received before.
		//TODO: OPTIMIZATION: use pointers instead of copying dccInfos.
		dccInfoCache[dccInfo.accesscategory()]=dccInfo;

	}
}

void LDM::receiveCamInfo() {
	string serializedCamInfo;

	while (1) {
		pair<string, string> received = mReceiverCamInfo->receive();
		serializedCamInfo = received.second;
		camInfoCache.ParseFromString(serializedCamInfo);

		stringstream sSql;
		// set decimal precision to 15
		sSql  << setprecision(15);

		sSql << "INSERT INTO CamInfo (time, triggerReason, delta) ";
		sSql << "VALUES (" << camInfoCache.time() << ", '" << camInfoCache.triggerreason() << "', " << camInfoCache.delta() << " );";
		mCamInfoMutex.lock();
		insert(sSql.str());
		mCamInfoMutex.unlock();

	}
}

int main(int argc, const char* argv[]) {

	LDM ldm;
	ldm.init();

	return EXIT_SUCCESS;
}
