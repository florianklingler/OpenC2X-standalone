#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include "ldm.h"
#include <unistd.h>
#include <string>
#include <iostream>

using namespace std;

INITIALIZE_EASYLOGGINGPP

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
   int i;
   for(i=0; i<argc; i++){
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}

LDM::LDM() {
	mReceiverFromCa = new CommunicationReceiver("Ldm", "8888", "CAM");
	mReceiverFromDen = new CommunicationReceiver("Ldm", "9999", "DENM");
	mLogger = new LoggingUtility("LDM");

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



void LDM::init() {
	mThreadReceiveFromCa = new boost::thread(&LDM::receiveFromCa, this);
	mThreadReceiveFromDen = new boost::thread(&LDM::receiveFromDen, this);
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

void LDM::insertCam(camPackage::CAM cam) {
	char* errmsg = 0;
	stringstream sSql;

	//insert GPS
	if (cam.has_gps()) {
		sSql << "INSERT INTO GPS (latitude, longitude, altitude, epx, epy, time, online, satellites) VALUES (" << cam.gps().latitude() << ", " << cam.gps().longitude() << ", " << cam.gps().altitude() << ", " << cam.gps().epx() << ", " << cam.gps().epy() << ", " << cam.gps().time() << ", " << cam.gps().online() << ", " << cam.gps().satellites() << " );";
		if (sqlite3_exec(mDb, sSql.str().c_str(), callback, 0, &errmsg) != SQLITE_OK) {
			mLogger->logError("SQL error");
		}
		sSql.str("");
		sSql.clear();
	}

	//insert OBD2
	if (cam.has_obd2()) {
		sSql << "INSERT INTO OBD2 (time, speed, rpm) VALUES (" << cam.obd2().time() << ", " << cam.obd2().speed() << ", " << cam.obd2().rpm() << " );";
		if (sqlite3_exec(mDb, sSql.str().c_str(), callback, 0, &errmsg) != SQLITE_OK) {
			mLogger->logError("SQL error");
		}
		sSql.str("");
		sSql.clear();
	}

	//insert CAM
	sSql << "INSERT INTO CAM (id, content, createTime) VALUES (" << cam.id() << ", '" << cam.content() << "', " << cam.createtime() << " );";
	if (sqlite3_exec(mDb, sSql.str().c_str(), callback, 0, &errmsg) != SQLITE_OK) {
		mLogger->logError("SQL error");
	}
	sSql.str("");
	sSql.clear();
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
