#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include "caservice.h"
#include <google/protobuf/text_format.h>
#include <unistd.h>
#include <iostream>
#include <ctime>
#include <chrono>
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

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
	wrapperPackage::WRAPPER wrapper;

	while (1) {
		pair<string, string> received = mReceiverFromDcc->receive();
		envelope = received.first;
		byteMessage = received.second;			//serialized WRAPPER

		wrapper.ParseFromString(byteMessage);	//deserialize WRAPPER
		byteMessage = wrapper.content();		//serialized CAM
		logDelay(byteMessage);

		cout << "forward incoming CAM to LDM" << endl;
		mSenderToLdm->send(envelope, byteMessage);	//send serialized CAM to LDM
	}
}

void CaService::receiveGpsData() {
	while (1) {
		string data = mReceiverGps->receiveGpsData();
		cout << "GPS DATA: " << data << endl;
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
	wrapperPackage::WRAPPER wrapper;

	cam = generateCam();
	wrapper = generateWrapper(cam);
	wrapper.SerializeToString(&byteMessage);
	cout << "send new CAM to LDM and DCC" << endl;
	mSenderToLdm->send("CAM", wrapper.content()); //send serialized CAM to LDM
	mSenderToDcc->send("CAM", byteMessage);	//send serialized WRAPPER to DCC
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

wrapperPackage::WRAPPER CaService::generateWrapper(camPackage::CAM cam) {
	wrapperPackage::WRAPPER wrapper;
	string serializedCam;

	//serialize CAM
	cam.SerializeToString(&serializedCam);

	//create WRAPPER
	wrapper.set_id(cam.id());
	wrapper.set_type(wrapperPackage::WRAPPER_Type_CAM);
	wrapper.set_priority(wrapperPackage::WRAPPER_Priority_BE);

	wrapper.set_createtime(cam.createtime());
	wrapper.set_validuntil(cam.createtime() + 1*1000*1000*1000);
	wrapper.set_content(serializedCam);

	return wrapper;
}

void CaConfig::loadConfigXML(const string &filename) {
	boost::property_tree::ptree pt;
	read_xml(filename, pt);
	mCamTriggerInterval = pt.get("cam.TriggerInterval", 500);
}

int main() {
	CaConfig config;
	try {
		// TODO: set proper path to config.xml
		// Right now, pwd is cam/Debug while running cam
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
