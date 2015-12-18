#include "caservice.h"

#include <google/protobuf/text_format.h>
#include <unistd.h>
#include <iostream>
#include <ctime>
#include <chrono>
#include <string>

INITIALIZE_EASYLOGGINGPP

using namespace std;

CaService::CaService() {
	mReceiverFromDcc = new CommunicationReceiver("CaService", "5555", "CAM");
	mSenderToDcc = new CommunicationSender("CaService", "6666");
	mSenderToLdm = new CommunicationSender("CaService", "8888");

	mLogger = new LoggingUtility("CaService");

	mIdCounter = 0;

	mCamTriggerInterval = 500;
	mTimer = new boost::asio::deadline_timer(mIoService,
			boost::posix_time::millisec(mCamTriggerInterval));
}

CaService::~CaService() {
	mThreadReceive->join();
	//mThreadSend->join();
}

void CaService::init() {
	mThreadReceive = new boost::thread(&CaService::receive, this);
	//mThreadSend = new boost::thread(&CaService::send, this);

	mTimer->async_wait(
			boost::bind(&CaService::triggerCam, this,
					boost::asio::placeholders::error));
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

//log delay of received CAM
void CaService::logDelay(string byteMessage) {
	camPackage::CAM cam;
	cam.ParseFromString(byteMessage);
	int64_t createTime = cam.createtime();
	int64_t receiveTime =
			chrono::high_resolution_clock::now().time_since_epoch()
					/ chrono::nanoseconds(1);
	int64_t delay = receiveTime - createTime;
	mLogger->logStats("CAM", cam.id(), delay);
}

//periodically generate CAMs and send to LDM and DCC
void CaService::send() {
	string byteMessage;
	camPackage::CAM cam;
	wrapperPackage::WRAPPER wrapper;

	long how_many = 1000;
	while (how_many--) {
		microSleep(10000);
		cam = generateCam();
		wrapper = generateWrapper(cam);
		wrapper.SerializeToString(&byteMessage);
		cout << "send new CAM to LDM and DCC" << endl;
		mSenderToLdm->send("CAM", wrapper.content()); //send serialized CAM to LDM
		mSenderToDcc->send("CAM", byteMessage);	//send serialized WRAPPER to DCC
	}
}

void CaService::triggerCam(const boost::system::error_code &ec) {
	string byteMessage;
	camPackage::CAM cam;
	wrapperPackage::WRAPPER wrapper;

	cam = generateCam();
	wrapper = generateWrapper(cam);
	wrapper.SerializeToString(&byteMessage);
	cout << "send new CAM to LDM and DCC" << endl;
	mSenderToLdm->send("CAM", wrapper.content());	//send serialized CAM to LDM
	mSenderToDcc->send("CAM", byteMessage);	//send serialized WRAPPER to DCC

	mTimer->expires_from_now(boost::posix_time::millisec(mCamTriggerInterval));
	mTimer->async_wait(
			boost::bind(&CaService::triggerCam, this,
					boost::asio::placeholders::error));
}

//generate new CAM with increasing ID and current timestamp
camPackage::CAM CaService::generateCam() {
	camPackage::CAM cam;

	//create CAM
	cam.set_id(mIdCounter++);
	cam.set_content("CAM from CA service");
	cam.set_createtime(
			chrono::high_resolution_clock::now().time_since_epoch()
					/ chrono::nanoseconds(1));

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
	wrapper.set_content(serializedCam);

	return wrapper;
}

void CaService::microSleep(double microSeconds) {

	time_t sleep_sec = (time_t) (((int) microSeconds) / (1000 * 1000));
	long sleep_nanosec = ((long) (microSeconds * 1000)) % (1000 * 1000 * 1000);

	struct timespec time[1];
	time[0].tv_sec = sleep_sec;
	time[0].tv_nsec = sleep_nanosec;
	nanosleep(time, NULL);
}

int main() {
	CaService cam;
	cam.init();

	return EXIT_SUCCESS;
}
