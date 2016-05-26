#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include "ldm.h"
#include <unistd.h>
#include <iostream>

using namespace std;

INITIALIZE_EASYLOGGINGPP

LDM::LDM() {
	mReceiverFromCa = new CommunicationReceiver("Ldm", "8888", "CAM");
	mReceiverFromDen = new CommunicationReceiver("Ldm", "9999", "DENM");
	mLogger = new LoggingUtility("LDM");

	//open SQLite database
	if(sqlite3_open("../db/ldm.db", &mDb)) {
		mLogger->logError("Cannot open database");
		sqlite3_close(mDb);
	}
	else {
		mLogger->logInfo("Opened database successfully");
	}
}

LDM::~LDM() {
	mThreadReceiveFromCa->join();
	mThreadReceiveFromDen->join();
	delete mThreadReceiveFromCa;
	delete mThreadReceiveFromDen;

	delete mReceiverFromCa;
	delete mReceiverFromDen;

	sqlite3_close(mDb);
}

//SQLite functions

//TODO: select last_insert_rowid();
//TODO: improve modularization, separation of SQL from functionality

//executes specified insert
void LDM::insert(string sqlCommand) {
	char* errmsg = 0;

	if (sqlite3_exec(mDb, sqlCommand.c_str(), NULL, 0, &errmsg) != SQLITE_OK) {
		string error(errmsg);
		mLogger->logError("SQL error: " + error);
		sqlite3_free(errmsg);
	}
}

//executes specified select on GPS table and returns result rows as list of GPS data	//TODO: add for cam, denm, obd2
list<gpsPackage::GPS> LDM::gpsSelect(string sqlCommand) {
	list<gpsPackage::GPS> result;
	result.clear();
	sqlite3_stmt *stmt;
	char* errmsg = 0;

	if (sqlite3_prepare_v2(mDb, sqlCommand.c_str(), -1, &stmt, NULL) == SQLITE_OK) {
		while (sqlite3_step(stmt) == SQLITE_ROW) {		//iterate over result rows
			gpsPackage::GPS gps;

			//set attributes
			gps.set_latitude(sqlite3_column_double(stmt, 1));
			gps.set_longitude(sqlite3_column_double(stmt, 2));
			gps.set_altitude(sqlite3_column_double(stmt, 3));
			gps.set_epx(sqlite3_column_double(stmt, 4));
			gps.set_epy(sqlite3_column_double(stmt, 5));
			gps.set_time(sqlite3_column_int64(stmt, 6));
			gps.set_online(sqlite3_column_int64(stmt, 7));
			gps.set_satellites(sqlite3_column_int(stmt, 8));

			result.push_back(gps);	//add to result

			//print for testing
			string textMessage;
			google::protobuf::TextFormat::PrintToString(gps, &textMessage);
			cout << textMessage << endl;
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

//inserts CAM into DB
void LDM::insertCam(camPackage::CAM cam) {
	stringstream sSql;

	//insert GPS if available
	if (cam.has_gps()) {
		sSql << "INSERT INTO GPS (latitude, longitude, altitude, epx, epy, time, online, satellites) VALUES (" << cam.gps().latitude() << ", " << cam.gps().longitude() << ", " << cam.gps().altitude() << ", " << cam.gps().epx() << ", " << cam.gps().epy() << ", " << cam.gps().time() << ", " << cam.gps().online() << ", " << cam.gps().satellites() << " );";
		insert(sSql.str());
		sSql.str("");
		sSql.clear();
	}

	//insert OBD2 if available
	if (cam.has_obd2()) {
		sSql << "INSERT INTO OBD2 (time, speed, rpm) VALUES (" << cam.obd2().time() << ", " << cam.obd2().speed() << ", " << cam.obd2().rpm() << " );";
		insert(sSql.str());
		sSql.str("");
		sSql.clear();
	}

	//insert CAM
	sSql << "INSERT INTO CAM (id, content, createTime) VALUES (" << cam.id() << ", '" << cam.content() << "', " << cam.createtime() << " );";
	insert(sSql.str());
	sSql.str("");
	sSql.clear();
}

//LDM functions

void LDM::init() {
	mThreadReceiveFromCa = new boost::thread(&LDM::receiveFromCa, this);
	mThreadReceiveFromDen = new boost::thread(&LDM::receiveFromDen, this);

//	gpsSelect("SELECT * from GPS");	//for testing
}

void LDM::receiveFromCa() {
	string serializedCam;	//serialized CAM
	string textMessage;		//text string (human readable)

	camPackage::CAM cam;

	while (1) {
		pair<string, string> received = mReceiverFromCa->receive();	//receive
		serializedCam = received.second;

		//print CAM
		cam.ParseFromString(serializedCam);
		google::protobuf::TextFormat::PrintToString(cam, &textMessage);
		mLogger->logInfo("received CAM:\n" + textMessage);

		insertCam(cam);
	}
}

void LDM::receiveFromDen() {
	string serializedDenm;		//serialized DENM
	string textMessage;		//text string (human readable)

	denmPackage::DENM denm;

	while (1) {
		pair<string, string> received = mReceiverFromDen->receive();//receive
		serializedDenm = received.second;

		//print DENM
		denm.ParseFromString(serializedDenm);
		google::protobuf::TextFormat::PrintToString(denm, &textMessage);
		mLogger->logInfo("received DENM:\n" + textMessage);
	}
}

int main() {
	LDM ldm;
	ldm.init();

	return EXIT_SUCCESS;
}
