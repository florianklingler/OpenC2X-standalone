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
	string byteMessage;		//byte string (serialized)
	dataPackage::DATA data;

	while (1) {
		pair<string, string> received = mReceiverFromDcc->receive();
		envelope = received.first;
		byteMessage = received.second;			//serialized DATA

		data.ParseFromString(byteMessage);	//deserialize DATA
		byteMessage = data.content();		//serialized CAM
		logDelay(byteMessage);

		cout << "forward incoming CAM to LDM" << endl;
		mSenderToLdm->send(envelope, byteMessage);	//send serialized CAM to LDM
	}
}

void CaService::receiveGpsData() {
	string serializedGps;
	gpsPackage::GPS gps;

	while (1) {
		serializedGps = mReceiverGps->receiveGpsData();
		gps.ParseFromString(serializedGps);
		cout << "Received GPS with latitude: " << gps.latitude() << ", longitude: " << gps.longitude() << endl;
	}
}

//log delay of received CAM
void CaService::logDelay(string byteMessage) {
	camPackage::CAM cam;
	cam.ParseFromString(byteMessage);
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
	string byteMessage;
	camPackage::CAM cam;
	dataPackage::DATA data;

	cam = generateCam();
	data = generateData(cam);
	data.SerializeToString(&byteMessage);
	cout << "send new CAM to LDM and DCC" << endl;
	mSenderToLdm->send("CAM", data.content()); //send serialized CAM to LDM
	mSenderToDcc->send("CAM", byteMessage);	//send serialized DATA to DCC
}

//generate new CAM with increasing ID and current timestamp
camPackage::CAM CaService::generateCam() {
	camPackage::CAM cam;

	//create CAM
	cam.set_id(mIdCounter++);
	cam.set_content("CAM from CA service");
	cam.set_createtime(chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1));

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
