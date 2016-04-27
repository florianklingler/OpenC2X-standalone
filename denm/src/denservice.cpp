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

using namespace std;

INITIALIZE_EASYLOGGINGPP

DenService::DenService() {
	string module = "DenService";
	mReceiverFromDcc = new CommunicationReceiver(module, "5555", "DENM");
	mSenderToDcc = new CommunicationSender(module, "7777");
	mSenderToLdm = new CommunicationSender(module, "9999");

	mReceiverGps = new CommunicationReceiver(module, "3333", "GPS");
	mReceiverObd2 = new CommunicationReceiver(module, "2222", "OBD2");

	mLogger = new LoggingUtility("DenService");

	mIdCounter = 0;
}

DenService::~DenService() {
	mThreadReceive->join();
	mThreadGpsDataReceive->join();
	mThreadObd2DataReceive->join();
	mThreadSend->join();
	delete mThreadReceive;
	delete mThreadGpsDataReceive;
	delete mThreadObd2DataReceive;
	delete mThreadSend;

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

	mThreadSend = new boost::thread(&DenService::triggerDenm, this);
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

		cout << "forward incoming DENM to LDM" << endl;
		mSenderToLdm->send(envelope, serializedData);
	}
}

void DenService::receiveGpsData() {
	string serializedGps;
	gpsPackage::GPS newGps;

	while (1) {
		serializedGps = mReceiverGps->receiveData();
		newGps.ParseFromString(serializedGps);
		cout << "Received GPS with latitude: " << newGps.latitude() << ", longitude: " << newGps.longitude() << endl;
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
		cout << "Received OBD2 with speed (m/s): " << newObd2.speed() << endl;
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
	int64_t receiveTime =
			chrono::high_resolution_clock::now().time_since_epoch()
					/ chrono::nanoseconds(1);
	int64_t delay = receiveTime - createTime;
	mLogger->logStats("DENM", denm.id(), delay);
}

//trigger sending to LDM and DCC every 100 to 1000ms to simulate road safety application
void DenService::triggerDenm() {
	while (1) {
		int randomSleep = rand() % 900 + 101;
		microSleep(randomSleep*1000);
		send();
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
void DenService::send() {
	string serializedData;
	denmPackage::DENM denm;
	dataPackage::DATA data;

	denm = generateDenm();
	data = generateData(denm);
	data.SerializeToString(&serializedData);
	cout << "send new DENM to LDM and DCC" << endl;
	mSenderToLdm->send("DENM", data.content());		//send serialized DENM to LDM
	mSenderToDcc->send("DENM", serializedData);		//send serialized DATA to DCC
}

//generate new DENM with increasing ID and current timestamp
denmPackage::DENM DenService::generateDenm() {
	denmPackage::DENM denm;

	//create DENM
	denm.set_id(mIdCounter++);
	denm.set_content("DENM from DEN service");
	denm.set_createtime(chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1));

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
	data.set_validuntil(denm.createtime() + 1*1000*1000*1000);	//1s
	data.set_content(serializedDenm);

	return data;
}

int main() {
	DenService denm;
	denm.init();

	return EXIT_SUCCESS;
}
