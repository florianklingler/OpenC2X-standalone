// This file is part of OpenC2X.
//
// OpenC2X is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenC2X is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OpenC2X.  If not, see <http://www.gnu.org/licenses/>.
//
// Authors:
// Sven Laux <slaux@mail.uni-paderborn.de>
// Gurjashan Singh Pannu <gurjashan.pannu@ccs-labs.org>
// Stefan Schneider <stefan.schneider@ccs-labs.org>
// Jan Tiemann <janhentie@web.de>


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
#include <utility/Utils.h>
#include <asn1/per_encoder.h>

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

	mLastCamTime = Utils::currentTime();
	mMsgUtils = new MessageUtils("CaService", mGlobalConfig.mExpNo);
	mLogger = new LoggingUtility("CaService", mGlobalConfig.mExpNo);
	mLogger->logStats("Station Id \tCAM id \tCreate Time \tReceive Time");

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

	delete mMsgUtils;

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

	camInfo.set_time(Utils::currentTime());
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
	int64_t receiveTime = Utils::currentTime();
	mLogger->logStats(cam.stationid() + "\t" + to_string(cam.id()) + "\t" + Utils::readableTime(createTime) + "\t" + Utils::readableTime(receiveTime));
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
	int64_t currentTime = Utils::currentTime();
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
		double deltaHeading = currentHeading - mLastSentCam.heading();
		if (deltaHeading > 180) {
			deltaHeading -= 360;
		} else if (deltaHeading < -180) {
			deltaHeading += 360;
		}
		if(abs(deltaHeading) > 4.0) {
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
	int64_t currentTime = Utils::currentTime();
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
	int64_t currentTime = Utils::currentTime();
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
//  Project group sending CAM[
	cam = generateCam();
//	data = generateData(cam);
//	data.SerializeToString(&serializedData);
//	mLogger->logInfo("Send new CAM " + to_string(data.id()) + " to DCC and LDM\n");
//	mSenderToDcc->send("CAM", serializedData);	//send serialized DATA to DCC
//	mSenderToLdm->send("CAM", data.content()); //send serialized CAM to LDM
//
	mLastSentCam = cam;
//]

	// Standard compliant CAM
	vector<uint8_t> encodedCam = generateCam2();
	string strCam(encodedCam.begin(), encodedCam.end());
	cout << "Encoded CAM size " << encodedCam.size() << " and string len: " << strCam.length() << endl;

	data.set_id(cam.id());
	data.set_type(dataPackage::DATA_Type_CAM);
	data.set_priority(dataPackage::DATA_Priority_BE);

	data.set_createtime(cam.createtime());
	data.set_validuntil(cam.createtime() + mConfig.mExpirationTime*1000*1000*1000);
	data.set_content(strCam);

	data.SerializeToString(&serializedData);
	mLogger->logInfo("Send new CAM " + to_string(data.id()) + " to DCC and LDM\n");

	mSenderToDcc->send("CAM", serializedData);	//send serialized DATA to DCC
	mSenderToLdm->send("CAM", data.content()); //send serialized CAM to LDM
//
//	mLastSentCam = cam;
//
//	CAM_t* decodedcam = 0;
//	mMsgUtils->decodeMessage(&asn_DEF_CAM, (void **)&decodedcam, strCam);
////	asn_fprint(stdout, &asn_DEF_CAM, decodedcam);
}

//generate new CAM with increasing ID, current timestamp and latest gps data
camPackage::CAM CaService::generateCam() {
	camPackage::CAM cam;

	//create CAM
	cam.set_stationid(mGlobalConfig.mMac);
	cam.set_id(mIdCounter++);
	cam.set_content("CAM from CA service");
	cam.set_createtime(Utils::currentTime());

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

vector<uint8_t> CaService::generateCam2() {
	cout << "generateCAM2  " << endl; //<< mGlobalConfig.mStationID << endl;
	CAM_t* cam = static_cast<CAM_t*>( calloc(1, sizeof(CAM_t)) );
	if (!cam) {
		throw runtime_error("could not allocate CAM_t");
	}
	cout << " local " << cam << " and size : " << sizeof(*cam) << endl;
	// ITS pdu header
	//TODO: GSP: station id is 0..4294967295
	cam->header.stationID = mIdCounter; //mGlobalConfig.mStationID;
	cam->header.messageID = messageID_cam;
	cam->header.protocolVersion = protocolVersion_currentVersion;

	// generation delta time
	cam->cam.generationDeltaTime = 0;

	// Basic container
	cam->cam.camParameters.basicContainer.stationType = StationType_unknown;
	cam->cam.camParameters.basicContainer.referencePosition.latitude = 0;
	cam->cam.camParameters.basicContainer.referencePosition.longitude = 0;
	cam->cam.camParameters.basicContainer.referencePosition.altitude.altitudeValue = 0;
	cam->cam.camParameters.basicContainer.referencePosition.altitude.altitudeConfidence = 0;
	cam->cam.camParameters.basicContainer.referencePosition.positionConfidenceEllipse.semiMajorConfidence = 0;
	cam->cam.camParameters.basicContainer.referencePosition.positionConfidenceEllipse.semiMajorOrientation = 0;
	cam->cam.camParameters.basicContainer.referencePosition.positionConfidenceEllipse.semiMinorConfidence = 0;

	// High frequency container
	// Could be basic vehicle or RSU and have corresponding details
	cam->cam.camParameters.highFrequencyContainer.present = HighFrequencyContainer_PR_basicVehicleContainerHighFrequency;

	cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.curvature.curvatureValue = CurvatureValue_unavailable;
	cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.curvature.curvatureConfidence = CurvatureConfidence_unavailable;

	cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.curvatureCalculationMode = CurvatureCalculationMode_unavailable;

	cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.driveDirection = DriveDirection_unavailable;

	cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.heading.headingValue = HeadingValue_unavailable;
	cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.heading.headingConfidence = HeadingConfidence_unavailable;

	cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.longitudinalAcceleration.longitudinalAccelerationValue = LongitudinalAccelerationValue_unavailable;
	cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.longitudinalAcceleration.longitudinalAccelerationConfidence = AccelerationConfidence_unavailable;

	cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.speed.speedValue = SpeedValue_unavailable;
	cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.speed.speedConfidence = SpeedConfidence_unavailable;

	cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.vehicleLength.vehicleLengthValue = VehicleLengthValue_unavailable;
	cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.vehicleLength.vehicleLengthConfidenceIndication = VehicleLengthConfidenceIndication_unavailable;

	cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.vehicleWidth = VehicleWidth_unavailable;


	cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.yawRate.yawRateValue = YawRateValue_unavailable;
	cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.yawRate.yawRateConfidence = YawRateConfidence_unavailable;
	//	cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.accelerationControl->
	//	cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.cenDsrcTollingZone->
	//	cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.verticalAcceleration
	//	cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.steeringWheelAngle
	//	cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.performanceClass
	//	cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.lanePosition->
	//	cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.lateralAcceleration

	// Optional part
	//	cam->cam.camParameters.lowFrequencyContainer->present =
	//	cam->cam.camParameters.specialVehicleContainer->present =

	// Encode message
    vector<uint8_t> encodedCam = mMsgUtils->encodeMessage(&asn_DEF_CAM, cam);

    // Printing the cam structure
    // asn_fprint(stdout, &asn_DEF_CAM, cam);

    //TODO: Free the allocated structure for cam. Is this enough?
    asn_DEF_CAM.free_struct(&asn_DEF_CAM, cam, 0);
    return encodedCam;
}

dataPackage::DATA CaService::generateData(string encodedCam) {
	dataPackage::DATA data;
	string serializedCam;

	//create DATA
	data.set_id(/*cam.id()*/1);
	data.set_type(dataPackage::DATA_Type_CAM);
	data.set_priority(dataPackage::DATA_Priority_BE);

	uint64_t time = Utils::currentTime();
	data.set_createtime(time);
	data.set_validuntil(time + mConfig.mExpirationTime*1000*1000*1000);
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
