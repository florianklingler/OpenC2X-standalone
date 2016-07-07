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

CaService::CaService(CaServiceConfig &config) {
	try {
		mGlobalConfig.loadConfigXML("../../common/config/config.xml");
	}
	catch (std::exception &e) {
		cerr << "Error while loading config.xml: " << e.what() << endl;
	}

	mConfig = config;
	mLogger = new LoggingUtility("CaService", mGlobalConfig.mExpNo);

	mReceiverFromDcc = new CommunicationReceiver("CaService", "5555", "CAM", mGlobalConfig.mExpNo);
	mSenderToDcc = new CommunicationSender("CaService", "6666", mGlobalConfig.mExpNo);
	mSenderToLdm = new CommunicationSender("CaService", "8888", mGlobalConfig.mExpNo);

	mReceiverGps = new CommunicationReceiver("CaService", "3333", "GPS", mGlobalConfig.mExpNo);
	mReceiverObd2 = new CommunicationReceiver("CaService", "2222", "OBD2", mGlobalConfig.mExpNo);

	mThreadReceive = new boost::thread(&CaService::receive, this);
	mThreadGpsDataReceive = new boost::thread(&CaService::receiveGpsData, this);
	mThreadObd2DataReceive = new boost::thread(&CaService::receiveObd2Data, this);

	mIdCounter = 0;

	mGpsValid = false;	//initially no data is available => not valid
	mObd2Valid = false;

	if (mConfig.mGenerateMsgs) {
		mTimer = new boost::asio::deadline_timer(mIoService, boost::posix_time::millisec(100));
		mTimer->async_wait(boost::bind(&CaService::triggerCam, this, boost::asio::placeholders::error));
		mIoService.run();
	}
	else {
		mLogger->logInfo("CAM triggering disabled");
	}
}

CaService::~CaService() {
	mThreadReceive->join();
	mThreadGpsDataReceive->join();
	mThreadObd2DataReceive->join();
	delete mThreadReceive;
	delete mThreadGpsDataReceive;
	delete mThreadObd2DataReceive;

	delete mReceiverFromDcc;
	delete mSenderToDcc;
	delete mSenderToLdm;

	delete mReceiverGps;
	delete mReceiverObd2;

	delete mLogger;

	mTimer->cancel();
	delete mTimer;
}

//receive CAM from DCC and forward to LDM
void CaService::receive() {
	string envelope;		//envelope
	string serializedData;	//byte string (serialized)
	dataPackage::DATA data;

	while (1) {
		pair<string, string> received = mReceiverFromDcc->receive();
		envelope = received.first;
		serializedData = received.second;			//serialized DATA

		data.ParseFromString(serializedData);	//deserialize DATA
		serializedData = data.content();		//serialized CAM
		logDelay(serializedData);

		mLogger->logInfo("Forward incoming CAM " + to_string(data.id()) + " to LDM");
		mSenderToLdm->send(envelope, serializedData);	//send serialized CAM to LDM
	}
}

void CaService::receiveGpsData() {
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

void CaService::receiveObd2Data() {
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

//sends info about triggering to LDM
void CaService::sendCamInfo(string triggerReason, double delta) {
	infoPackage::CamInfo camInfo;
	string serializedCamInfo;

	camInfo.set_time(chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1));
	camInfo.set_triggerreason(triggerReason);
	camInfo.set_delta(delta);

	camInfo.SerializeToString(&serializedCamInfo);
	mSenderToLdm->send("camInfo", serializedCamInfo);
}

//log delay of received CAM
void CaService::logDelay(string serializedCam) {
	camPackage::CAM cam;
	cam.ParseFromString(serializedCam);
	int64_t createTime = cam.createtime();
	int64_t receiveTime = chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1);
	int64_t delay = receiveTime - createTime;
	mLogger->logStats(to_string(cam.id()) + "\t" + to_string(delay));
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
	if (getDistance(lat1, lon1, lat2, lon2) < mConfig.mThresholdRadiusForHeading) {
		mLogger->logDebug("Ignore heading: not moved more than " +to_string(mConfig.mThresholdRadiusForHeading) + " meters.");
		return -1;
	}

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
	// Check heading and position conditions only if we have valid GPS data
	if(isGPSdataValid()) {

		if(!mLastSentCam.has_gps()) {
			sendCamInfo("First GPS data", -1);
			mLogger->logInfo("First GPS data");
			send();
			scheduleNextTrigger();
			return;
		}

		//|current position - last CAM position| > 5 m
		if(isPositionChanged()) {
			send();
			scheduleNextTrigger();
			return;
		}

		//|current heading (towards North) - last CAM heading| > 4 deg
		if(isHeadingChanged()) {
			send();
			scheduleNextTrigger();
			return;
		}
	}

	//|current speed - last CAM speed| > 1 m/s
	if(isSpeedChanged()) {
		send();
		scheduleNextTrigger();
		return;
	}

	//max. time interval 1s
	if(isTimeToTriggerCAM()) {
		send();
		scheduleNextTrigger();
		return;
	}

	scheduleNextTrigger();
}

bool CaService::isGPSdataValid() {
	mMutexLatestGps.lock();
	int64_t currentTime = chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1);
	if (currentTime - mLatestGps.time() > (int64_t)mConfig.mMaxGpsAge * 1000*1000*1000) {	//GPS data too old
		mGpsValid = false;
	} else {
		mGpsValid = true;
	}
	mMutexLatestGps.unlock();
	return mGpsValid;
}

bool CaService::isHeadingChanged() {
	mMutexLatestGps.lock();
	double currentHeading = getHeading(mLastSentCam.gps().latitude(), mLastSentCam.gps().longitude(), mLatestGps.latitude(), mLatestGps.longitude());
	if(currentHeading != -1) {
		double deltaHeading = abs(currentHeading - mLastSentCam.heading());
		if(deltaHeading > 4.0) {
			sendCamInfo("heading", deltaHeading);
			mLogger->logInfo("deltaHeading: " + to_string(deltaHeading));
			mMutexLatestGps.unlock();
			return true;
		}
	}
	mMutexLatestGps.unlock();
	return false;
}

bool CaService::isPositionChanged() {
	mMutexLatestGps.lock();
	double distance = getDistance(mLastSentCam.gps().latitude(), mLastSentCam.gps().longitude(), mLatestGps.latitude(), mLatestGps.longitude());
	if(distance > 5.0) {
		sendCamInfo("distance", distance);
		mLogger->logInfo("distance: " + to_string(distance));
		mMutexLatestGps.unlock();
		return true;
	}
	mMutexLatestGps.unlock();
	return false;
}

bool CaService::isSpeedChanged() {
	mMutexLatestObd2.lock();
	int64_t currentTime = chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1);
	if (currentTime - mLatestObd2.time() > (int64_t)mConfig.mMaxObd2Age * 1000*1000*1000) {	//OBD2 data too old
		mMutexLatestObd2.unlock();
		mObd2Valid = false;
		return false;
	}
	mObd2Valid = true;
	double deltaSpeed = abs(mLatestObd2.speed() - mLastSentCam.obd2().speed());
	if(deltaSpeed > 1.0) {
		sendCamInfo("speed", deltaSpeed);
		mLogger->logInfo("deltaSpeed: " + to_string(deltaSpeed));
		mMutexLatestObd2.unlock();
		return true;
	}
	mMutexLatestObd2.unlock();
	return false;
}

bool CaService::isTimeToTriggerCAM() {
	//max. time interval 1s
	int64_t currentTime = chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1);
	int64_t deltaTime = currentTime - mLastSentCam.createtime();
	if(deltaTime >= 1*1000*1000*1000) {
		sendCamInfo("time", deltaTime);
		mLogger->logInfo("deltaTime: " + to_string(deltaTime));
		return true;
	}
	return false;
}

void CaService::scheduleNextTrigger() {
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
	mLogger->logInfo("Send new CAM " + to_string(data.id()) + " to DCC and LDM\n");
	mSenderToDcc->send("CAM", serializedData);	//send serialized DATA to DCC
	mSenderToLdm->send("CAM", data.content()); //send serialized CAM to LDM

	mLastSentCam = cam;
}

//generate new CAM with increasing ID, current timestamp and latest gps data
camPackage::CAM CaService::generateCam() {
	camPackage::CAM cam;

	//create CAM
	cam.set_stationid(mGlobalConfig.mMac);
	cam.set_id(mIdCounter++);
	cam.set_content("CAM from CA service");
	cam.set_createtime(chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1));

	mMutexLatestGps.lock();
	if(mGpsValid) {														//only add gps if valid data is available
		gpsPackage::GPS* gps = new gpsPackage::GPS(mLatestGps);		//data needs to be copied to a new buffer because new gps data can be received before sending
		cam.set_allocated_gps(gps);

		double currentHeading = getHeading(mLastSentCam.gps().latitude(), mLastSentCam.gps().longitude(), mLatestGps.latitude(), mLatestGps.longitude());
		cam.set_heading(currentHeading);
	}
	mMutexLatestGps.unlock();

	mMutexLatestObd2.lock();
	if(mObd2Valid) {													//only add obd2 if valid data is available
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
	data.set_validuntil(cam.createtime() + mConfig.mExpirationTime*1000*1000*1000);
	data.set_content(serializedCam);

	return data;
}

int main() {
	CaServiceConfig config;
	try {
		config.loadConfigXML("../config/config.xml");
	}
	catch (std::exception &e) {
		cerr << "Error while loading config.xml: " << e.what() << endl << flush;
		return EXIT_FAILURE;
	}
	CaService cam(config);

	return EXIT_SUCCESS;
}
