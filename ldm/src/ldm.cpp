#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include "ldm.h"
#include <unistd.h>
#include <iostream>

using namespace std;

INITIALIZE_EASYLOGGINGPP

LDM::LDM() {
	string moduleName = "Ldm";
	mReceiverFromCa = new CommunicationReceiver(moduleName, "8888", "CAM");
	mReceiverFromDen = new CommunicationReceiver(moduleName, "9999", "DENM");
	mReceiverDccInfo = new CommunicationReceiver(moduleName, "1234", "dccInfo");
	mReceiverCamInfo = new CommunicationReceiver(moduleName, "8888", "camInfo");
	mServer = new CommunicationServer(moduleName, "6789");
	mLogger = new LoggingUtility(moduleName);

	//open SQLite database
	if(sqlite3_open("../db/ldm.db", &mDb)) {
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
			"key INTEGER PRIMARY KEY, id INTEGER, content TEXT, createTime INTEGER, gps INTEGER, obd2 INTEGER, " \
			"FOREIGN KEY(gps) REFERENCES GPS (KEYWORDASCOLUMNNAME), FOREIGN KEY(obd2) REFERENCES OBD2 (KEYWORDASCOLUMNNAME));";
	if (sqlite3_exec(mDb, sqlCommand, NULL, 0, &errmsg)) {
		string error(errmsg);
		mLogger->logError("SQL error: " + error);
		sqlite3_free(errmsg);
	}

	//create DENM table
	sqlCommand = "CREATE TABLE IF NOT EXISTS DENM(" \
				"key INTEGER PRIMARY KEY, id INTEGER, content TEXT, createTime INTEGER, gps INTEGER, obd2 INTEGER, " \
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
				"key INTEGER PRIMARY KEY, time INTEGER, channelLoad REAL, state TEXT, AC TEXT, availableTokens INTEGER, queuedPackets INTEGER, dccMechanism INTEGER, txPower REAL, tokenInterval REAL, datarate REAL, carrierSense REAL);";
	if (sqlite3_exec(mDb, sqlCommand, NULL, 0, &errmsg)) {
		string error(errmsg);
		mLogger->logError("SQL error: " + error);
		sqlite3_free(errmsg);
	}

	//create CamInfo table
	sqlCommand = "CREATE TABLE IF NOT EXISTS CamInfo(" \
				"key INTEGER PRIMARY KEY, time INTEGER, tirggerReason TEXT, delta REAL);";
	if (sqlite3_exec(mDb, sqlCommand, NULL, 0, &errmsg)) {
		string error(errmsg);
		mLogger->logError("SQL error: " + error);
		sqlite3_free(errmsg);
	}
}

//executes SELECT with specified condition (eg. WHERE) on GPS table and returns result rows as list of GPS data
list<gpsPackage::GPS> LDM::gpsSelect(string condition) {
	list<gpsPackage::GPS> result;
	result.clear();
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

			result.push_back(gps);	//add to result
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

//executes specified SELECT with specified condition (eg. WHERE) on OBD2 table and returns result rows as list of OBD2 data
list<obd2Package::OBD2> LDM::obd2Select(string condition) {
	list<obd2Package::OBD2> result;
	result.clear();
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

			result.push_back(obd2);	//add to result
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

//executes specified SELECT with specified condition (eg. WHERE) on CAM table and returns result rows as list of CAMs
list<camPackage::CAM> LDM::camSelect(string condition) {
	list<camPackage::CAM> result;
	result.clear();
	sqlite3_stmt *stmt;
	string sqlCommand = "SELECT * from CAM " + condition;
	char* errmsg = 0;

	if (sqlite3_prepare_v2(mDb, sqlCommand.c_str(), -1, &stmt, NULL) == SQLITE_OK) {
		while (sqlite3_step(stmt) == SQLITE_ROW) {		//iterate over result rows
			camPackage::CAM cam;

			//set attributes retrieved from result columns
			cam.set_id(sqlite3_column_int(stmt, 1));
			cam.set_content(string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))));
			cam.set_createtime(sqlite3_column_int64(stmt, 3));

			//add GPS if available
			int64_t gpsRowId = sqlite3_column_int64(stmt, 4);
			if (gpsRowId > 0) {
				list<gpsPackage::GPS> gpsList = gpsSelect("WHERE key=" + to_string(gpsRowId));
				gpsPackage::GPS* gps = new gpsPackage::GPS(gpsList.front());
				cam.set_allocated_gps(gps);
			}

			//add OBD2 if available
			int64_t obd2RowId = sqlite3_column_int64(stmt, 5);
			if (obd2RowId > 0) {
				list<obd2Package::OBD2> obd2List = obd2Select("WHERE key=" + to_string(obd2RowId));
				obd2Package::OBD2* obd2 = new obd2Package::OBD2(obd2List.front());
				cam.set_allocated_obd2(obd2);
			}

			result.push_back(cam);	//add to result
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

//executes specified SELECT with specified condition (eg. WHERE) on DENM table and returns result rows as list of DENMs
list<denmPackage::DENM> LDM::denmSelect(string condition) {
	list<denmPackage::DENM> result;
	result.clear();
	sqlite3_stmt *stmt;
	string sqlCommand = "SELECT * from DENM " + condition;
	char* errmsg = 0;

	if (sqlite3_prepare_v2(mDb, sqlCommand.c_str(), -1, &stmt, NULL) == SQLITE_OK) {
		while (sqlite3_step(stmt) == SQLITE_ROW) {		//iterate over result rows
			denmPackage::DENM denm;

			//set attributes retrieved from result columns
			denm.set_id(sqlite3_column_int(stmt, 1));
			denm.set_content(string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))));
			denm.set_createtime(sqlite3_column_int64(stmt, 3));

			//add GPS if available
			int64_t gpsRowId = sqlite3_column_int64(stmt, 4);
			if (gpsRowId > 0) {
				list<gpsPackage::GPS> gpsList = gpsSelect("WHERE key=" + to_string(gpsRowId));
				gpsPackage::GPS* gps = new gpsPackage::GPS(gpsList.front());
				denm.set_allocated_gps(gps);
			}

			//add OBD2 if available
			int64_t obd2RowId = sqlite3_column_int64(stmt, 5);
			if (obd2RowId > 0) {
				list<obd2Package::OBD2> obd2List = obd2Select("WHERE key=" + to_string(obd2RowId));
				obd2Package::OBD2* obd2 = new obd2Package::OBD2(obd2List.front());
				denm.set_allocated_obd2(obd2);
			}

			result.push_back(denm);	//add to result
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
		sSql << "INSERT INTO CAM (id, content, createTime, gps, obd2) VALUES (" << cam.id() << ", '" << cam.content() << "', " << cam.createtime() << ", " << gpsRowId << ", " << obd2RowId << " );";
	}
	else if (gpsRowId > 0) {
		sSql << "INSERT INTO CAM (id, content, createTime, gps) VALUES (" << cam.id() << ", '" << cam.content() << "', " << cam.createtime() << ", " << gpsRowId << " );";
	}
	else if (obd2RowId > 0) {
		sSql << "INSERT INTO CAM (id, content, createTime, obd2) VALUES (" << cam.id() << ", '" << cam.content() << "', " << cam.createtime() << ", " << obd2RowId << " );";
	}
	else {
		sSql << "INSERT INTO CAM (id, content, createTime) VALUES (" << cam.id() << ", '" << cam.content() << "', " << cam.createtime() << " );";
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
		sSql << "INSERT INTO DENM (id, content, createTime, gps, obd2) VALUES (" << denm.id() << ", '" << denm.content() << "', " << denm.createtime() << ", " << gpsRowId << ", " << obd2RowId << " );";
	}
	else if (gpsRowId > 0) {
		sSql << "INSERT INTO DENM (id, content, createTime, gps) VALUES (" << denm.id() << ", '" << denm.content() << "', " << denm.createtime() << ", " << gpsRowId << " );";
	}
	else if (obd2RowId > 0) {
		sSql << "INSERT INTO DENM (id, content, createTime, obd2) VALUES (" << denm.id() << ", '" << denm.content() << "', " << denm.createtime() << ", " << obd2RowId << " );";
	}
	else {
		sSql << "INSERT INTO DENM (id, content, createTime) VALUES (" << denm.id() << ", '" << denm.content() << "', " << denm.createtime() << " );";
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
	stream << "CAM - " << readableTime(cam.createtime()) << ", id: " << cam.id() << ", content: " << cam.content();
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
	stream << "DENM - " << readableTime(denm.createtime()) << ", id: " << denm.id() << ", content: " << denm.content();
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
	string request, reply;
	mLogger->logDebug("waiting for request");
	while(1) {
		request = mServer->receiveRequest();
		//TODO: process request
		sleep(1);
		reply = request;
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
		sSql << "INSERT INTO DccInfo (time, channelLoad, state, AC, availableTokens, queuedPackets, dccMechanism, txPower, tokenInterval, datarate, carrierSense) ";
		sSql << "VALUES (" << dccInfo.time() << ", " << dccInfo.channelload() << ", '" << dccInfo.state() << "', '" << dccInfo.accesscategory() << "', " << dccInfo.availabletokens() << ", " << dccInfo.queuedpackets() << ", " << dccInfo.dccmechanism() << ", " << dccInfo.txpower() << ", " << dccInfo.tokeninterval() << ", " << dccInfo.datarate() << ", " << dccInfo.carriersense() << " );";
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
