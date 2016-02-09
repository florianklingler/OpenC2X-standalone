#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include "caservice.h"
#include <google/protobuf/text_format.h>
#include <unistd.h>
#include <iostream>
#include <ctime>
#include <chrono>
#include <cmath>
#include <string>

using namespace std;

INITIALIZE_EASYLOGGINGPP

CaService::CaService() {
	string module = "CaService";
	mReceiverFromDcc = new CommunicationReceiver(module, "5555", "CAM");
	mSenderToDcc = new CommunicationSender(module, "6666");
	mSenderToLdm = new CommunicationSender(module, "8888");

	mReceiverGps = new CommunicationReceiver(module, "3333", "GPS");

	mLogger = new LoggingUtility("CaService");

	mIdCounter = 0;

	mTimer = new boost::asio::deadline_timer(mIoService, boost::posix_time::millisec(100));
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

void CaService::receiveObd2Data() {
	string serializedGps;
	obd2Package::OBD2 newObd2;

	while (1) {
		//serializedGps = mReceiverGps->receiveGpsData();	//TODO: implement OBD2
		//newObd2.ParseFromString(serializedGps);
		newObd2.set_speed(10.0);
		cout << "Received Obd2 with speed: " << newObd2.speed() << endl;
		mMutexLatestObd2.lock();
		mLatestObd2 = newObd2;
		mMutexLatestObd2.unlock();
		sleep(1);
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

double CaService::getDistance(double lat1, double lon1, double lat2, double lon2) {
	double R = 6371; // km
	double dLat = (lat2-lat1) * M_PI/180.0;		//convert to rad
	double dLon = (lon2-lon1) * M_PI/180.0;
	lat1 = lat1 * M_PI/180.0;
	lat2 = lat2 * M_PI/180.0;

	double a = sin(dLat/2) * sin(dLat/2) + sin(dLon/2) * sin(dLon/2) * cos(lat1) * cos(lat2);
	double c = 2 * atan2(sqrt(a), sqrt(1-a));
	return R * c * 1000;						//convert to m
}

double CaService::getHeading(double lat1, double lon1, double lat2, double lon2) {
	double dLat = (lat2-lat1) * M_PI/180.0;		//convert to rad
	double dLon = (lon2-lon1) * M_PI/180.0;
	lat1 = lat1 * M_PI/180.0;

	double phi = atan2(sin(lat1)*dLon, dLat);	//between -pi and +pi
	phi = phi * 180.0/M_PI;						//convert to deg (-180, +180)
	if(phi < 0) {
		phi += 360;								//between 0 and 360 deg
	}
	return phi;
}

//periodically check generation rules for sending to LDM and DCC
void CaService::triggerCam(const boost::system::error_code &ec) {
	bool sendCam = false;

	//max. time interval 1s
	int64_t currentTime = chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1);
	int64_t deltaTime = currentTime - mLastSentCam.createtime();
	if(deltaTime >= 1*1000*1000*1000) {
		cout << "deltaTime: " << deltaTime << endl;
		sendCam = true;
	}

	//|current heading (towards North) - last CAM heading| > 4 deg
	mMutexLatestGps.lock();
	double currentHeading = getHeading(mLastSentCam.gps().latitude(), mLastSentCam.gps().longitude(), mLatestGps.latitude(), mLatestGps.longitude());
	double deltaHeading = abs(currentHeading - mLastSentCam.heading());
	if(deltaHeading > 4.0) {
		cout << "deltaHeading: " << deltaHeading << endl;
		sendCam = true;
	}
	mMutexLatestGps.unlock();

	//|current position - last CAM position| > 5 m
	mMutexLatestGps.lock();
	double distance = getDistance(mLastSentCam.gps().latitude(), mLastSentCam.gps().longitude(), mLatestGps.latitude(), mLatestGps.longitude());
	if(distance > 5.0) {
		cout << "distance: " << distance << endl;
		sendCam = true;
	}
	mMutexLatestGps.unlock();

	//|current speed - last CAM speed| > 1 m/s
	mMutexLatestObd2.lock();
	double deltaSpeed = abs(mLatestObd2.speed() - mLastSentCam.obd2().speed());
	if(deltaSpeed > 1.0) {
		cout << "deltaSpeed: " << deltaSpeed << endl;
		sendCam = true;
	}
	mMutexLatestObd2.unlock();

	if(sendCam) {
		send();
	}

	//min. time interval 0.1 s
	mTimer->expires_from_now(boost::posix_time::millisec(100));
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
	cout << "send new CAM to LDM and DCC\n" << endl;
	mSenderToLdm->send("CAM", data.content()); //send serialized CAM to LDM
	mSenderToDcc->send("CAM", serializedData);	//send serialized DATA to DCC

	mLastSentCam = cam;
}

//generate new CAM with increasing ID, current timestamp and latest gps data
camPackage::CAM CaService::generateCam() {
	camPackage::CAM cam;

	//create CAM
	cam.set_id(mIdCounter++);
	cam.set_content("CAM from CA service");
	cam.set_createtime(chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1));

	mMutexLatestGps.lock();
	if(mLatestGps.has_time()) {		//only add gps if valid data is available
		gpsPackage::GPS* gps = new gpsPackage::GPS(mLatestGps);	//data needs to be copied to a new buffer because new gps data can be received before sending
		cam.set_allocated_gps(gps);

		double currentHeading = getHeading(mLastSentCam.gps().latitude(), mLastSentCam.gps().longitude(), mLatestGps.latitude(), mLatestGps.longitude());
		cam.set_heading(currentHeading);
	}
	mMutexLatestGps.unlock();

	mMutexLatestObd2.lock();
	if(mLatestObd2.has_time()) {									//only add obd2 if valid data is available
		obd2Package::OBD2* obd2 = new obd2Package::OBD2(mLatestObd2);	//data needs to be copied to a new buffer because new obd2 data can be received before sending
		cam.set_allocated_obd2(obd2);
		//TODO: delete obd2, gps?
	}
	mMutexLatestObd2.unlock();

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
	CaService cam;
	cam.init();

	return EXIT_SUCCESS;
}
