#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include "denservice.h"
#include <google/protobuf/text_format.h>
#include <unistd.h>
#include <iostream>
#include <ctime>
#include <chrono>
#include <string>
#include <stdlib.h>
#include <utility/Utils.h>

using namespace std;

INITIALIZE_EASYLOGGINGPP

DenService::DenService() {
	try {
		mGlobalConfig.loadConfigXML("../../common/config/config.xml");
	}
	catch (std::exception &e) {
		cerr << "Error while loading config.xml: " << e.what() << endl;
	}

	string module = "DenService";
	mReceiverFromApp = new CommunicationReceiver(module, "1111", "TRIGGER", mGlobalConfig.mExpNo);
	mReceiverFromDcc = new CommunicationReceiver(module, "5555", "DENM", mGlobalConfig.mExpNo);
	mSenderToDcc = new CommunicationSender(module, "7777", mGlobalConfig.mExpNo);
	mSenderToLdm = new CommunicationSender(module, "9999", mGlobalConfig.mExpNo);

	mReceiverGps = new CommunicationReceiver(module, "3333", "GPS", mGlobalConfig.mExpNo);
	mReceiverObd2 = new CommunicationReceiver(module, "2222", "OBD2", mGlobalConfig.mExpNo);

	mLogger = new LoggingUtility("DenService", mGlobalConfig.mExpNo);
	mLogger->logStats("Station Id \tDENM id \tCreate Time \tReceive Time");

	mIdCounter = 0;
}

DenService::~DenService() {
	mThreadReceive->join();
	mThreadGpsDataReceive->join();
	mThreadObd2DataReceive->join();
	mThreadAppTrigger->join();
	delete mThreadReceive;
	delete mThreadGpsDataReceive;
	delete mThreadObd2DataReceive;
	delete mThreadAppTrigger;

	delete mReceiverFromApp;
	delete mReceiverFromDcc;
	delete mSenderToDcc;
	delete mSenderToLdm;

	delete mReceiverGps;
	delete mReceiverObd2;

	delete mLogger;
}

void DenService::init() {
	mThreadReceive = new boost::thread(&DenService::receive, this);
	mThreadGpsDataReceive = new boost::thread(&DenService::receiveGpsData, this);
	mThreadObd2DataReceive = new boost::thread(&DenService::receiveObd2Data, this);
	mThreadAppTrigger = new boost::thread(&DenService::triggerAppDenm, this);
}

//receive DENM from DCC and forward to LDM
void DenService::receive() {
	string envelope;			//envelope
	string serializedData;		//serialized DATA
	dataPackage::DATA data;

	while (1) {
		pair<string, string> received = mReceiverFromDcc->receive();
		envelope = received.first;
		serializedData = received.second;

		data.ParseFromString(serializedData);	//deserialize DATA
		serializedData = data.content();		//serialized DENM
		logDelay(serializedData);

		mLogger->logInfo("forward incoming DENM " + to_string(data.id()) + " to LDM");
		mSenderToLdm->send(envelope, serializedData);
	}
}

void DenService::receiveGpsData() {
	string serializedGps;
	gpsPackage::GPS newGps;

	while (1) {
		serializedGps = mReceiverGps->receiveData();
		newGps.ParseFromString(serializedGps);
		mLogger->logDebug("Received GPS with latitude: " + to_string(newGps.latitude()) + ", longitude: " + to_string(newGps.longitude()));
		mMutexLatestGps.lock();
		mLatestGps = newGps;
		mMutexLatestGps.unlock();
	}
}

void DenService::receiveObd2Data() {
	string serializedObd2;
	obd2Package::OBD2 newObd2;

	while (1) {
		serializedObd2 = mReceiverObd2->receiveData();
		newObd2.ParseFromString(serializedObd2);
		mLogger->logDebug("Received OBD2 with speed (m/s): " + to_string(newObd2.speed()));
		mMutexLatestObd2.lock();
		mLatestObd2 = newObd2;
		mMutexLatestObd2.unlock();
	}
}

//log delay of received DENM
void DenService::logDelay(string serializedDenm) {
	denmPackage::DENM denm;
	denm.ParseFromString(serializedDenm);
	int64_t createTime = denm.createtime();
	int64_t receiveTime = Utils::currentTime();
	mLogger->logStats(denm.stationid() + "\t" + to_string(denm.id()) + "\t" + Utils::readableTime(createTime) + "\t" + Utils::readableTime(receiveTime));
}

//trigger generation/send of DENM by external application
void DenService::triggerAppDenm() {
	string envelope;
	string serializedTrigger;
	triggerPackage::TRIGGER trigger;

	while(1) {
		pair<string, string> received = mReceiverFromApp->receive();
		envelope = received.first;
		serializedTrigger = received.second;
		trigger.ParseFromString(serializedTrigger);

		send(trigger);
	}
}

void DenService::microSleep(double microSeconds) {
	time_t sleep_sec = (time_t) (((int) microSeconds) / (1000 * 1000));
	long sleep_nanosec = ((long) (microSeconds * 1000)) % (1000 * 1000 * 1000);

	struct timespec time[1];
	time[0].tv_sec = sleep_sec;
	time[0].tv_nsec = sleep_nanosec;
	nanosleep(time, NULL);
}

//generate DENM and send to LDM and DCC
void DenService::send(triggerPackage::TRIGGER trigger) {
	string serializedData;
	denmPackage::DENM denm;
	dataPackage::DATA data;

	denm = generateDenm(trigger);
	data = generateData(denm);
	data.SerializeToString(&serializedData);
	mLogger->logInfo("send new DENM " + to_string(data.id()) + " to DCC and LDM");
	mSenderToDcc->send("DENM", serializedData);		//send serialized DATA to DCC
	mSenderToLdm->send("DENM", data.content());		//send serialized DENM to LDM
}

//generate new DENM with increasing ID and current timestamp
denmPackage::DENM DenService::generateDenm(triggerPackage::TRIGGER trigger) {
	denmPackage::DENM denm;

	//create DENM
	denm.set_stationid(mGlobalConfig.mMac);
	denm.set_id(mIdCounter++);
	denm.set_content(trigger.content());
	denm.set_createtime(Utils::currentTime());

	mMutexLatestGps.lock();
	if(mLatestGps.has_time()) {											//only add gps if valid data is available
		gpsPackage::GPS* gps = new gpsPackage::GPS(mLatestGps);			//data needs to be copied to a new buffer because new gps data can be received before sending
		denm.set_allocated_gps(gps);
	}
	mMutexLatestGps.unlock();

	mMutexLatestObd2.lock();
	if(mLatestObd2.has_time()) {										//only add obd2 if valid data is available
		obd2Package::OBD2* obd2 = new obd2Package::OBD2(mLatestObd2);	//data needs to be copied to a new buffer because new obd2 data can be received before sending
		denm.set_allocated_obd2(obd2);
		//TODO: delete obd2, gps?
	}
	mMutexLatestObd2.unlock();

	return denm;
}

dataPackage::DATA DenService::generateData(denmPackage::DENM denm) {
	dataPackage::DATA data;
	string serializedDenm;

	//serialize DENM
	denm.SerializeToString(&serializedDenm);

	//create DATA
	data.set_id(denm.id());
	data.set_type(dataPackage::DATA_Type_DENM);
	data.set_priority(dataPackage::DATA_Priority_VI);

	data.set_createtime(denm.createtime());
	data.set_validuntil(denm.createtime() + 1*1000*1000*1000);	//1s TODO: conform to standard? -> specify using CLI
	data.set_content(serializedDenm);

	return data;
}

int main() {
	DenService denm;
	denm.init();

	return EXIT_SUCCESS;
}
