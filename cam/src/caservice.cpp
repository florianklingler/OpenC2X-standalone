#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include "caservice.h"
#include <google/protobuf/text_format.h>
#include <unistd.h>
#include <iostream>
#include <ctime>
#include <chrono>
#include <string>

using namespace std;

INITIALIZE_EASYLOGGINGPP

CaService::CaService(CaConfig &config) {
	string module = "CaService";
	mConfig = config;
	mReceiverFromDcc = new CommunicationReceiver(module, "5555", "CAM");
	mSenderToDcc = new CommunicationSender(module, "6666");
	mSenderToLdm = new CommunicationSender(module, "8888");

	mReceiverGps = new CommunicationReceiver(module, "3333", "GPS");

	mLogger = new LoggingUtility("CaService");

	mIdCounter = 0;

	mCamTriggerInterval = mConfig.mCamTriggerInterval;
	mTimer = new boost::asio::deadline_timer(mIoService, boost::posix_time::millisec(mCamTriggerInterval));
}

CaService::~CaService() {
	mThreadReceive->join();
	mThreadGpsDataReceive->join();
	delete mThreadReceive;
	delete mThreadGpsDataReceive;

	delete mReceiverFromDcc;
	delete mSenderToDcc;
	delete mSenderToLdm;

	delete mReceiverGps;

	delete mLogger;

	mTimer->cancel();
	delete mTimer;
}

void CaService::init() {
	mThreadReceive = new boost::thread(&CaService::receive, this);
	mThreadGpsDataReceive = new boost::thread(&CaService::receiveGpsData, this);

	mTimer->async_wait(boost::bind(&CaService::triggerCam, this, boost::asio::placeholders::error));
	mIoService.run();
}

//receive CAM from DCC and forward to LDM
void CaService::receive() {
	string envelope;		//envelope
	string serializedData;		//byte string (serialized)
	dataPackage::DATA data;

	while (1) {
		pair<string, string> received = mReceiverFromDcc->receive();
		envelope = received.first;
		serializedData = received.second;			//serialized DATA

		data.ParseFromString(serializedData);	//deserialize DATA
		serializedData = data.content();		//serialized CAM
		logDelay(serializedData);

		cout << "forward incoming CAM to LDM" << endl;
		mSenderToLdm->send(envelope, serializedData);	//send serialized CAM to LDM
	}
}

void CaService::receiveGpsData() {
	string serializedGps;
	gpsPackage::GPS newGps;

	while (1) {
		serializedGps = mReceiverGps->receiveGpsData();
		newGps.ParseFromString(serializedGps);
		cout << "Received GPS with latitude: " << newGps.latitude() << ", longitude: " << newGps.longitude() << endl;
		mMutexLatestGps.lock();
		mLatestGps = newGps;
		mMutexLatestGps.unlock();
	}
}

//log delay of received CAM
void CaService::logDelay(string serializedCam) {
	camPackage::CAM cam;
	cam.ParseFromString(serializedCam);
	int64_t createTime = cam.createtime();
	int64_t receiveTime = chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1);
	int64_t delay = receiveTime - createTime;
	mLogger->logStats("CAM", cam.id(), delay);
}

//periodically trigger sending to LDM and DCC
void CaService::triggerCam(const boost::system::error_code &ec) {
	send();

	mTimer->expires_from_now(boost::posix_time::millisec(mCamTriggerInterval));
	mTimer->async_wait(boost::bind(&CaService::triggerCam, this, boost::asio::placeholders::error));
}

//generate CAM and send to LDM and DCC
void CaService::send() {
	string serializedData;
	camPackage::CAM cam;
	dataPackage::DATA data;

	cam = generateCam();
	data = generateData(cam);
	data.SerializeToString(&serializedData);
	cout << "send new CAM to LDM and DCC" << endl;
	mSenderToLdm->send("CAM", data.content()); //send serialized CAM to LDM
	mSenderToDcc->send("CAM", serializedData);	//send serialized DATA to DCC
}

//generate new CAM with increasing ID, current timestamp and latest gps data
camPackage::CAM CaService::generateCam() {
	camPackage::CAM cam;

	//create CAM
	cam.set_id(mIdCounter++);
	cam.set_content("CAM from CA service");
	cam.set_createtime(chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1));
	if(mLatestGps.has_time()) {									//only add gps if valid data is available
		mMutexLatestGps.lock();
		gpsPackage::GPS* gps = new gpsPackage::GPS(mLatestGps);	//data needs to be copied to a new buffer because new gps data can be received before sending
		mMutexLatestGps.unlock();
		cam.set_allocated_gps(gps);
	}

	return cam;
}

dataPackage::DATA CaService::generateData(camPackage::CAM cam) {
	dataPackage::DATA data;
	string serializedCam;

	//serialize CAM
	cam.SerializeToString(&serializedCam);

	//create DATA
	data.set_id(cam.id());
	data.set_type(dataPackage::DATA_Type_CAM);
	data.set_priority(dataPackage::DATA_Priority_BE);

	data.set_createtime(cam.createtime());
	data.set_validuntil(cam.createtime() + 1*1000*1000*1000);	//1s
	data.set_content(serializedCam);

	return data;
}

int main() {
	CaConfig config;
	try {
		config.loadConfigXML("../src/config.xml");
	}
	catch (std::exception &e) {
		cerr << "Error while loading config.xml: " << e.what() << endl << flush;
		return EXIT_FAILURE;
	}
	CaService cam(config);
	cam.init();

	return EXIT_SUCCESS;
}
