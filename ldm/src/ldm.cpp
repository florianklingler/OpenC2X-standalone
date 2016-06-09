#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include "ldm.h"
#include <unistd.h>
#include <iostream>
#include <config/config.h>

using namespace std;

INITIALIZE_EASYLOGGINGPP

LDM::LDM() {
	GlobalConfig config;
	try {
		config.loadConfigXML("../../common/config/config.xml");
	}
	catch (std::exception &e) {
		cerr << "Error while loading config.xml: " << e.what() << endl;
	}

	string moduleName = "Ldm";
	mReceiverFromCa = new CommunicationReceiver(moduleName, "8888", "CAM");
	mReceiverFromDen = new CommunicationReceiver(moduleName, "9999", "DENM");
	mReceiverDccInfo = new CommunicationReceiver(moduleName, "1234", "dccInfo");
	mReceiverCamInfo = new CommunicationReceiver(moduleName, "8888", "camInfo");
	mServer = new CommunicationServer(moduleName, "6789");
	mLogger = new LoggingUtility(moduleName);

	//open SQLite database
	if(sqlite3_open(("../db/ldm-" + to_string(config.mExpNo) + ".db").c_str(), &mDb)) {
		mLogger->logError("Cannot open database");
		sqlite3_close(mDb);
	}
	else {
		mLogger->logInfo("Opened database successfully");
		createTables();
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
	sqlCommand = "CREATE TABLE IF NOT EXISTS CAM(" \
			"key INTEGER PRIMARY KEY, stationId TEXT, id INTEGER, content TEXT, createTime INTEGER, gps INTEGER, obd2 INTEGER, " \
			"FOREIGN KEY(gps) REFERENCES GPS (KEYWORDASCOLUMNNAME), FOREIGN KEY(obd2) REFERENCES OBD2 (KEYWORDASCOLUMNNAME));";
	if (sqlite3_exec(mDb, sqlCommand, NULL, 0, &errmsg)) {
		string error(errmsg);
		mLogger->logError("SQL error: " + error);
		sqlite3_free(errmsg);
	}

	//create DENM table
	sqlCommand = "CREATE TABLE IF NOT EXISTS DENM(" \
				"key INTEGER PRIMARY KEY, stationId TEXT, id INTEGER, content TEXT, createTime INTEGER, gps INTEGER, obd2 INTEGER, " \
				"FOREIGN KEY(gps) REFERENCES GPS (KEYWORDASCOLUMNNAME), FOREIGN KEY(obd2) REFERENCES OBD2 (KEYWORDASCOLUMNNAME));";
	if (sqlite3_exec(mDb, sqlCommand, NULL, 0, &errmsg)) {
		string error(errmsg);
		mLogger->logError("SQL error: " + error);
		sqlite3_free(errmsg);
	}

	//create GPS table
	sqlCommand = "CREATE TABLE IF NOT EXISTS GPS(" \
				"key INTEGER PRIMARY KEY, latitude REAL, longitude REAL, altitude REAL, epx REAL, epy REAL, time INTEGER, online INTEGER, satellites INTEGER);";
	if (sqlite3_exec(mDb, sqlCommand, NULL, 0, &errmsg)) {
		string error(errmsg);
		mLogger->logError("SQL error: " + error);
		sqlite3_free(errmsg);
	}

	//create OBD2 table
	sqlCommand = "CREATE TABLE IF NOT EXISTS OBD2(" \
				"key INTEGER PRIMARY KEY, time INTEGER, speed REAL, rpm INTEGER);";
	if (sqlite3_exec(mDb, sqlCommand, NULL, 0, &errmsg)) {
		string error(errmsg);
		mLogger->logError("SQL error: " + error);
		sqlite3_free(errmsg);
	}

	//create DccInfo table
	sqlCommand = "CREATE TABLE IF NOT EXISTS DccInfo(" \
				"key INTEGER PRIMARY KEY, time INTEGER, channelLoad REAL, state TEXT, AC TEXT, availableTokens INTEGER, queuedPackets INTEGER, dccMechanism INTEGER, txPower REAL, tokenInterval REAL, datarate REAL, carrierSense REAL, flushReqPackets INTEGER, flushNotReqPackets INTEGER);";
	if (sqlite3_exec(mDb, sqlCommand, NULL, 0, &errmsg)) {
		string error(errmsg);
		mLogger->logError("SQL error: " + error);
		sqlite3_free(errmsg);
	}

	//create CamInfo table
	sqlCommand = "CREATE TABLE IF NOT EXISTS CamInfo(" \
				"key INTEGER PRIMARY KEY, time INTEGER, triggerReason TEXT, delta REAL);";
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

	return result;
}

//executes specified SELECT with specified condition (eg. WHERE) on OBD2 table and returns result
dataPackage::LdmData LDM::obd2Select(string condition) {
	dataPackage::LdmData result;
	result.set_type(dataPackage::LdmData_Type_OBD2);
	sqlite3_stmt *stmt;
	string sqlCommand = "SELECT * from OBD2 " + condition;
	char* errmsg = 0;

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

	return result;
}

//executes specified SELECT with specified condition (eg. WHERE) on CAM table and returns result
dataPackage::LdmData LDM::camSelect(string condition) {
	dataPackage::LdmData result;
	result.set_type(dataPackage::LdmData_Type_CAM);
	sqlite3_stmt *stmt;
	string sqlCommand;
	if (condition == "latest") {	//only get the most recent CAM for each stationId
		//sql from http://stackoverflow.com/questions/1313120/retrieving-the-last-record-in-each-group; have to use createTime instead of id because id isn't necessarily unique and growing
		sqlCommand = "SELECT cam1.* from CAM cam1 LEFT JOIN CAM cam2 ON (cam1.stationId = cam2.stationId AND cam1.createTime < cam2.createTime) WHERE cam2.createTime IS NULL";
	}
	else {
		sqlCommand = "SELECT * from CAM " + condition;
	}

	char* errmsg = 0;

	if (sqlite3_prepare_v2(mDb, sqlCommand.c_str(), -1, &stmt, NULL) == SQLITE_OK) {
		while (sqlite3_step(stmt) == SQLITE_ROW) {		//iterate over result rows
			camPackage::CAM cam;

			//set attributes retrieved from result columns
			cam.set_stationid(string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))));
			cam.set_id(sqlite3_column_int(stmt, 2));
			cam.set_content(string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3))));
			cam.set_createtime(sqlite3_column_int64(stmt, 4));

			//add GPS if available
			int64_t gpsRowId = sqlite3_column_int64(stmt, 5);
			if (gpsRowId > 0) {
				dataPackage::LdmData ldmData = gpsSelect("WHERE key=" + to_string(gpsRowId));
				gpsPackage::GPS* gps = new gpsPackage::GPS();
				gps->ParseFromString(ldmData.data(0));
				cam.set_allocated_gps(gps);
			}

			//add OBD2 if available
			int64_t obd2RowId = sqlite3_column_int64(stmt, 6);
			if (obd2RowId > 0) {
				dataPackage::LdmData ldmData = obd2Select("WHERE key=" + to_string(obd2RowId));
				obd2Package::OBD2* obd2 = new obd2Package::OBD2();
				obd2->ParseFromString(ldmData.data(0));
				cam.set_allocated_obd2(obd2);
			}

			//add result
			string serializedCam;
			cam.SerializeToString(&serializedCam);
			result.add_data(serializedCam);
		}
		sqlite3_finalize(stmt);
	}
	else {
		string error(errmsg);
		mLogger->logError("SQL error: " + error);
		sqlite3_free(errmsg);
	}

	return result;
}

//executes specified SELECT with specified condition (eg. WHERE) on DENM table and returns result
dataPackage::LdmData LDM::denmSelect(string condition) {
	dataPackage::LdmData result;
	result.set_type(dataPackage::LdmData_Type_DENM);
	sqlite3_stmt *stmt;
	string sqlCommand;
	if (condition == "latest") {	//only get the latest DENM for each stationId
		sqlCommand = "SELECT denm1.* from DENM denm1 LEFT JOIN DENM denm2 ON (denm1.stationId = denm2.stationId AND denm1.createTime < denm2.createTime) WHERE denm2.createTime IS NULL";
	}
	else {
		sqlCommand = "SELECT * from DENM " + condition;
	}
	char* errmsg = 0;

	if (sqlite3_prepare_v2(mDb, sqlCommand.c_str(), -1, &stmt, NULL) == SQLITE_OK) {
		while (sqlite3_step(stmt) == SQLITE_ROW) {		//iterate over result rows
			denmPackage::DENM denm;

			//set attributes retrieved from result columns
			denm.set_stationid(string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))));
			denm.set_id(sqlite3_column_int(stmt, 2));
			denm.set_content(string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3))));
			denm.set_createtime(sqlite3_column_int64(stmt, 4));

			//add GPS if available
			int64_t gpsRowId = sqlite3_column_int64(stmt, 5);
			if (gpsRowId > 0) {
				dataPackage::LdmData ldmData = gpsSelect("WHERE key=" + to_string(gpsRowId));
				gpsPackage::GPS* gps = new gpsPackage::GPS();
				gps->ParseFromString(ldmData.data(0));
				denm.set_allocated_gps(gps);
			}

			//add OBD2 if available
			int64_t obd2RowId = sqlite3_column_int64(stmt, 6);
			if (obd2RowId > 0) {
				dataPackage::LdmData ldmData = obd2Select("WHERE key=" + to_string(obd2RowId));
				obd2Package::OBD2* obd2 = new obd2Package::OBD2();
				obd2->ParseFromString(ldmData.data(0));
				denm.set_allocated_obd2(obd2);
			}

			//add result
			string serializedDenm;
			denm.SerializeToString(&serializedDenm);
			result.add_data(serializedDenm);
		}
		sqlite3_finalize(stmt);
	}
	else {
		string error(errmsg);
		mLogger->logError("SQL error: " + error);
		sqlite3_free(errmsg);
	}

	return result;
}

//executes specified SELECT with specified condition (eg. WHERE) on dccInfo table and returns result
dataPackage::LdmData LDM::dccInfoSelect(string condition) {
	dataPackage::LdmData result;
	result.set_type(dataPackage::LdmData_Type_dccInfo);
	sqlite3_stmt *stmt;
	string sqlCommand;
	if (condition == "latest") {	//only get the latest dccInfo for each AC
		sqlCommand = "SELECT dccInfo1.* from DccInfo dccInfo1 LEFT JOIN DccInfo dccInfo2 ON (dccInfo1.AC = dccInfo2.AC AND dccInfo1.time < dccInfo2.time) WHERE dccInfo2.time IS NULL";
	}
	else {
		sqlCommand = "SELECT * from DccInfo " + condition;
	}
	char* errmsg = 0;

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

			//add to result
			string serializedDccInfo;
			dccInfo.SerializeToString(&serializedDccInfo);
			result.add_data(serializedDccInfo);
		}
		sqlite3_finalize(stmt);
	}
	else {
		string error(errmsg);
		mLogger->logError("SQL error: " + error);
		sqlite3_free(errmsg);
	}

	return result;
}

//executes specified SELECT with specified condition (eg. WHERE) on camInfo table and returns result
dataPackage::LdmData LDM::camInfoSelect(string condition) {
	dataPackage::LdmData result;
	result.set_type(dataPackage::LdmData_Type_camInfo);
	sqlite3_stmt *stmt;
	string sqlCommand;
	if (condition == "latest") {	//only get the latest camInfo
		sqlCommand = "SELECT * from CamInfo ORDER BY key DESC LIMIT 1";
	}
	else {
		sqlCommand = "SELECT * from CamInfo " + condition;
	}
	char* errmsg = 0;

	if (sqlite3_prepare_v2(mDb, sqlCommand.c_str(), -1, &stmt, NULL) == SQLITE_OK) {
		while (sqlite3_step(stmt) == SQLITE_ROW) {		//iterate over result rows
			infoPackage::CamInfo camInfo;

			//set attributes retrieved from result columns
			camInfo.set_time(sqlite3_column_int64(stmt, 1));
			camInfo.set_triggerreason(string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))));
			camInfo.set_delta(sqlite3_column_double(stmt, 3));

			//add to result
			string serializedCamInfo;
			camInfo.SerializeToString(&serializedCamInfo);
			result.add_data(serializedCamInfo);
		}
		sqlite3_finalize(stmt);
	}
	else {
		string error(errmsg);
		mLogger->logError("SQL error: " + error);
		sqlite3_free(errmsg);
	}

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
	int64_t gpsRowId = -1;
	int64_t obd2RowId = -1;

	//insert GPS if available
	if (cam.has_gps()) {
		sSql << "INSERT INTO GPS (latitude, longitude, altitude, epx, epy, time, online, satellites) VALUES (" << cam.gps().latitude() << ", " << cam.gps().longitude() << ", " << cam.gps().altitude() << ", " << cam.gps().epx() << ", " << cam.gps().epy() << ", " << cam.gps().time() << ", " << cam.gps().online() << ", " << cam.gps().satellites() << " );";
		insert(sSql.str());
		sSql.str("");
		sSql.clear();
		gpsRowId = sqlite3_last_insert_rowid(mDb);
	}

	//insert OBD2 if available
	if (cam.has_obd2()) {
		sSql << "INSERT INTO OBD2 (time, speed, rpm) VALUES (" << cam.obd2().time() << ", " << cam.obd2().speed() << ", " << cam.obd2().rpm() << " );";
		insert(sSql.str());
		sSql.str("");
		sSql.clear();
		obd2RowId = sqlite3_last_insert_rowid(mDb);
	}

	//insert CAM with foreign keys to reference GPS, OBD2
	if (gpsRowId > 0 && obd2RowId > 0) {
		sSql << "INSERT INTO CAM (stationId, id, content, createTime, gps, obd2) VALUES ('" << cam.stationid() << "', " << cam.id() << ", '" << cam.content() << "', " << cam.createtime() << ", " << gpsRowId << ", " << obd2RowId << " );";
	}
	else if (gpsRowId > 0) {
		sSql << "INSERT INTO CAM (stationId, id, content, createTime, gps) VALUES ('" << cam.stationid() << "', " << cam.id() << ", '" << cam.content() << "', " << cam.createtime() << ", " << gpsRowId << " );";
	}
	else if (obd2RowId > 0) {
		sSql << "INSERT INTO CAM (stationId, id, content, createTime, obd2) VALUES ('" << cam.stationid()  << "', " << cam.id() << ", '" << cam.content() << "', " << cam.createtime() << ", " << obd2RowId << " );";
	}
	else {
		sSql << "INSERT INTO CAM (stationId, id, content, createTime) VALUES ('" << cam.stationid() << "', " << cam.id() << ", '" << cam.content() << "', " << cam.createtime() << " );";
	}
	insert(sSql.str());
}

//inserts DENM into DB
void LDM::insertDenm(denmPackage::DENM denm) {
	stringstream sSql;
	int64_t gpsRowId = -1;
	int64_t obd2RowId = -1;

	//insert GPS if available
	if (denm.has_gps()) {
		sSql << "INSERT INTO GPS (latitude, longitude, altitude, epx, epy, time, online, satellites) VALUES (" << denm.gps().latitude() << ", " << denm.gps().longitude() << ", " << denm.gps().altitude() << ", " << denm.gps().epx() << ", " << denm.gps().epy() << ", " << denm.gps().time() << ", " << denm.gps().online() << ", " << denm.gps().satellites() << " );";
		insert(sSql.str());
		sSql.str("");
		sSql.clear();
		gpsRowId = sqlite3_last_insert_rowid(mDb);
	}

	//insert OBD2 if available
	if (denm.has_obd2()) {
		sSql << "INSERT INTO OBD2 (time, speed, rpm) VALUES (" << denm.obd2().time() << ", " << denm.obd2().speed() << ", " << denm.obd2().rpm() << " );";
		insert(sSql.str());
		sSql.str("");
		sSql.clear();
		obd2RowId = sqlite3_last_insert_rowid(mDb);
	}

	//insert CAM with foreign keys to reference GPS, OBD2
	if (gpsRowId > 0 && obd2RowId > 0) {
		sSql << "INSERT INTO DENM (stationId, id, content, createTime, gps, obd2) VALUES ('" << denm.stationid() << "', " << denm.id() << ", '" << denm.content() << "', " << denm.createtime() << ", " << gpsRowId << ", " << obd2RowId << " );";
	}
	else if (gpsRowId > 0) {
		sSql << "INSERT INTO DENM (stationId, id, content, createTime, gps) VALUES ('" << denm.stationid() << "', " << denm.id() << ", '" << denm.content() << "', " << denm.createtime() << ", " << gpsRowId << " );";
	}
	else if (obd2RowId > 0) {
		sSql << "INSERT INTO DENM (stationId, id, content, createTime, obd2) VALUES ('" << denm.stationid() << "', " << denm.id() << ", '" << denm.content() << "', " << denm.createtime() << ", " << obd2RowId << " );";
	}
	else {
		sSql << "INSERT INTO DENM (stationId, id, content, createTime) VALUES ('" << denm.stationid() << "', " << denm.id() << ", '" << denm.content() << "', " << denm.createtime() << " );";
	}
	insert(sSql.str());
}


//////////log/print function
//converts ns since epoch into human readable format (HH:MM:SS,ms)
string LDM::readableTime(int64_t nanoTime) {
	char buffer[9];
	int64_t milliTime = nanoTime / (1*1000*1000);		//ns to ms (since epoch)
	time_t epochTime = milliTime / 1000;				//ms to s (since epoch)
	struct tm* timeinfo = localtime(&epochTime);
	strftime(buffer, 9, "%T", timeinfo);				//buffer contains time HH:MM:SS
	int ms = milliTime % epochTime;						//just the ms

	stringstream time;								//convert to string
	time << buffer << "," << ms;

	return time.str();
}

void LDM::printGps(gpsPackage::GPS gps) {
	stringstream stream;

	stream << "GPS - " << readableTime(gps.time()) << ", lat: " << gps.latitude() << ", long: " << gps.longitude() << ", alt: " << gps.altitude();
	//TODO: include epx, epy, ...?

	mLogger->logInfo(stream.str());
}

void LDM::printObd2(obd2Package::OBD2 obd2) {
	stringstream stream;
	stream << "OBD2 - " << readableTime(obd2.time()) << ", speed: " << obd2.speed() << ", rpm: " << obd2.rpm();
	mLogger->logInfo(stream.str());
}

void LDM::printCam(camPackage::CAM cam) {
	stringstream stream;
	stream << "CAM - " << readableTime(cam.createtime()) << ", MAC: " << cam.stationid() << ", id: " << cam.id() << ", content: " << cam.content();
	if (cam.has_gps()) {

		stream << "\n\tGPS - " << readableTime(cam.gps().time()) << ", lat: " << cam.gps().latitude() << ", long: " << cam.gps().longitude() << ", alt: " << cam.gps().altitude();
	}
	if (cam.has_obd2()) {
		stream << "\n\tOBD2 - " << readableTime(cam.obd2().time()) << ", speed: " << cam.obd2().speed() << ", rpm: " << cam.obd2().rpm();
	}
	mLogger->logInfo(stream.str());
}

void LDM::printDenm(denmPackage::DENM denm) {
	stringstream stream;
	stream << "DENM - " << readableTime(denm.createtime()) << ", MAC: " << denm.stationid() << ", id: " << denm.id() << ", content: " << denm.content();
	if (denm.has_gps()) {

		stream << "\n\tGPS - " << readableTime(denm.gps().time()) << ", lat: " << denm.gps().latitude() << ", long: " << denm.gps().longitude() << ", alt: " << denm.gps().altitude();
	}
	if (denm.has_obd2()) {
		stream << "\n\tOBD2 - " << readableTime(denm.obd2().time()) << ", speed: " << denm.obd2().speed() << ", rpm: " << denm.obd2().rpm();
	}
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

		printCam(cam);
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

		printDenm(denm);
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
		sSql << "INSERT INTO DccInfo (time, channelLoad, state, AC, availableTokens, queuedPackets, dccMechanism, txPower, tokenInterval, datarate, carrierSense, flushReqPackets, flushNotReqPackets) ";
		sSql << "VALUES (" << dccInfo.time() << ", " << dccInfo.channelload() << ", '" << dccInfo.state() << "', '" << dccInfo.accesscategory() << "', " << dccInfo.availabletokens() << ", " << dccInfo.queuedpackets() << ", " << dccInfo.dccmechanism() << ", " << dccInfo.txpower() << ", " << dccInfo.tokeninterval() << ", " << dccInfo.datarate() << ", " << dccInfo.carriersense() << ", " << dccInfo.flushreqpackets() << ", " << dccInfo.flushnotreqpackets() << " );";
		insert(sSql.str());

		mLogger->logDebug("received dccInfo");
	}
}

void LDM::receiveCamInfo() {
	string serializedCamInfo;
	infoPackage::CamInfo camInfo;

	while (1) {
		pair<string, string> received = mReceiverCamInfo->receive();
		serializedCamInfo = received.second;
		camInfo.ParseFromString(serializedCamInfo);

		stringstream sSql;
		sSql << "INSERT INTO CamInfo (time, triggerReason, delta) ";
		sSql << "VALUES (" << camInfo.time() << ", '" << camInfo.triggerreason() << "', " << camInfo.delta() << " );";
		insert(sSql.str());

		mLogger->logDebug("received camInfo");
	}
}

int main() {
	LDM ldm;
	ldm.init();

	return EXIT_SUCCESS;
}
