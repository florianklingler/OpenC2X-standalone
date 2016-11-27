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
#include <common/utility/Utils.h>
#include <common/asn1/per_encoder.h>

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
		mTimer->async_wait(boost::bind(&CaService::alarm, this, boost::asio::placeholders::error));
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
	string serializedAsnCam;	//byte string (serialized)
	string serializedProtoCam;

	while (1) {
		pair<string, string> received = mReceiverFromDcc->receive();
		envelope = received.first;
		serializedAsnCam = received.second;			//serialized DATA

		CAM_t* cam = 0;
		int res = mMsgUtils->decodeMessage(&asn_DEF_CAM, (void **)&cam, serializedAsnCam);
		if (res != 0) {
			mLogger->logError("Failed to decode received CAM. Error code: " + to_string(res));
			continue;
		}
		//asn_fprint(stdout, &asn_DEF_CAM, cam);
		camPackage::CAM camProto = convertAsn1toProtoBuf(cam);
		camProto.SerializeToString(&serializedProtoCam);

		mLogger->logInfo("Forward incoming CAM " + to_string(cam->header.stationID) + " to LDM");
		mSenderToLdm->send(envelope, serializedProtoCam);	//send serialized CAM to LDM
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
void CaService::alarm(const boost::system::error_code &ec) {
	// Check heading and position conditions only if we have valid GPS data
	if(isGPSdataValid()) {
		if (!mLastSentCamInfo.hasGPS) {
			sendCamInfo("First GPS data", -1);
			mLogger->logInfo("First GPS data");
			trigger();
			return;
		}

		//|current position - last CAM position| > 5 m
		if(isPositionChanged()) {
			trigger();
			return;
		}

		//|current heading (towards North) - last CAM heading| > 4 deg
		if(isHeadingChanged()) {
			trigger();
			return;
		}
	}

	//|current speed - last CAM speed| > 1 m/s
	if(isSpeedChanged()) {
		trigger();
		return;
	}

	//max. time interval 1s
	if(isTimeToTriggerCAM()) {
		trigger();
		return;
	}

	scheduleNextAlarm();
}

void CaService::trigger() {
	send();
	scheduleNextAlarm();
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
	double currentHeading = getHeading(mLastSentCamInfo.lastGps.latitude(), mLastSentCamInfo.lastGps.longitude(), mLatestGps.latitude(), mLatestGps.longitude());
	if(currentHeading != -1) {
		double deltaHeading = currentHeading - mLastSentCamInfo.lastHeading;
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
	double distance = getDistance(mLastSentCamInfo.lastGps.latitude(), mLastSentCamInfo.lastGps.longitude(), mLatestGps.latitude(), mLatestGps.longitude());
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
	double deltaSpeed = abs(mLatestObd2.speed() - mLastSentCamInfo.lastObd2.speed());
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
	int64_t deltaTime = currentTime - mLastSentCamInfo.timestamp;
	if(deltaTime >= 1*1000*1000*1000) {
		sendCamInfo("time", deltaTime);
		mLogger->logInfo("deltaTime: " + to_string(deltaTime));
		return true;
	}
	return false;
}

void CaService::scheduleNextAlarm() {
	//min. time interval 0.1 s
	mTimer->expires_from_now(boost::posix_time::millisec(100));
	mTimer->async_wait(boost::bind(&CaService::alarm, this, boost::asio::placeholders::error));
}

//generate CAM and send to LDM and DCC
void CaService::send() {
	string serializedData;
	dataPackage::DATA data;

	// Standard compliant CAM
	CAM_t* cam = generateCam();
	vector<uint8_t> encodedCam = mMsgUtils->encodeMessage(&asn_DEF_CAM, cam);
	string strCam(encodedCam.begin(), encodedCam.end());
	mLogger->logDebug("Encoded CAM size: " + to_string(strCam.length()));

	data.set_id(messageID_cam);
	data.set_type(dataPackage::DATA_Type_CAM);
	data.set_priority(dataPackage::DATA_Priority_BE);

	int64_t currTime = Utils::currentTime();
	data.set_createtime(currTime);
	data.set_validuntil(currTime + mConfig.mExpirationTime*1000*1000*1000);
	data.set_content(strCam);

	data.SerializeToString(&serializedData);
	mLogger->logInfo("Send new CAM to DCC and LDM\n");

	mSenderToDcc->send("CAM", serializedData);	//send serialized DATA to DCC

	camPackage::CAM camProto = convertAsn1toProtoBuf(cam);
	string serializedProtoCam;
	camProto.SerializeToString(&serializedProtoCam);
	mSenderToLdm->send("CAM", serializedProtoCam); //send serialized CAM to LDM
    asn_DEF_CAM.free_struct(&asn_DEF_CAM, cam, 0);
}

//generate new CAM with latest gps and obd2 data
CAM_t* CaService::generateCam() {
	mLogger->logDebug("Generating CAM as per UPER");
	CAM_t* cam = static_cast<CAM_t*>(calloc(1, sizeof(CAM_t)));
	if (!cam) {
		throw runtime_error("could not allocate CAM_t");
	}
	// ITS pdu header
	cam->header.stationID = mGlobalConfig.mStationID;// mIdCounter; //
	cam->header.messageID = messageID_cam;
	cam->header.protocolVersion = protocolVersion_currentVersion;

	// generation delta time
	int64_t currTime = Utils::currentTime();
	if (mLastSentCamInfo.timestamp) {
		cam->cam.generationDeltaTime = (currTime - mLastSentCamInfo.timestamp) / (1000000);
	} else {
		cam->cam.generationDeltaTime = 0;
	}
	mLastSentCamInfo.timestamp = currTime;

	// Basic container
	cam->cam.camParameters.basicContainer.stationType = mConfig.mIsRSU ? StationType_roadSideUnit : StationType_passengerCar;

	mMutexLatestGps.lock();
	if (mGpsValid) {
		cam->cam.camParameters.basicContainer.referencePosition.latitude = mLatestGps.latitude() * 10000000; // in one-tenth of microdegrees
		cam->cam.camParameters.basicContainer.referencePosition.longitude = mLatestGps.longitude() * 10000000; // in one-tenth of microdegrees
		cam->cam.camParameters.basicContainer.referencePosition.altitude.altitudeValue = mLatestGps.altitude();
		double currentHeading = getHeading(mLastSentCamInfo.lastGps.latitude(), mLastSentCamInfo.lastGps.longitude(), mLatestGps.latitude(), mLatestGps.longitude());

		mLastSentCamInfo.hasGPS = true;
		mLastSentCamInfo.lastGps = gpsPackage::GPS(mLatestGps);		//data needs to be copied to a new buffer because new gps data can be received before sending
		mLastSentCamInfo.lastHeading = currentHeading;
	} else {
		mLastSentCamInfo.hasGPS = false;
		cam->cam.camParameters.basicContainer.referencePosition.latitude = Latitude_unavailable;
		cam->cam.camParameters.basicContainer.referencePosition.longitude = Longitude_unavailable;
		cam->cam.camParameters.basicContainer.referencePosition.altitude.altitudeValue = AltitudeValue_unavailable;
	}
	cam->cam.camParameters.basicContainer.referencePosition.altitude.altitudeConfidence = AltitudeConfidence_unavailable;
	mMutexLatestGps.unlock();

	cam->cam.camParameters.basicContainer.referencePosition.positionConfidenceEllipse.semiMajorConfidence = 0;
	cam->cam.camParameters.basicContainer.referencePosition.positionConfidenceEllipse.semiMajorOrientation = 0;
	cam->cam.camParameters.basicContainer.referencePosition.positionConfidenceEllipse.semiMinorConfidence = 0;

	// High frequency container
	// Could be basic vehicle or RSU and have corresponding details
	if(mConfig.mIsRSU) {
		cam->cam.camParameters.highFrequencyContainer.present = HighFrequencyContainer_PR_rsuContainerHighFrequency;
		// Optional fields in CAM from RSU
		// cam->cam.camParameters.highFrequencyContainer.choice.rsuContainerHighFrequency.protectedCommunicationZonesRSU
	} else {
		cam->cam.camParameters.highFrequencyContainer.present = HighFrequencyContainer_PR_basicVehicleContainerHighFrequency;
		cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.curvature.curvatureValue = CurvatureValue_unavailable;
		cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.curvature.curvatureConfidence = CurvatureConfidence_unavailable;

		cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.curvatureCalculationMode = CurvatureCalculationMode_unavailable;

		cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.driveDirection = DriveDirection_unavailable;

		cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.heading.headingValue = HeadingValue_unavailable;
		cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.heading.headingConfidence = HeadingConfidence_unavailable;

		cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.longitudinalAcceleration.longitudinalAccelerationValue = LongitudinalAccelerationValue_unavailable;
		cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.longitudinalAcceleration.longitudinalAccelerationConfidence = AccelerationConfidence_unavailable;


		mMutexLatestObd2.lock();
		if (mObd2Valid) {
			cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.speed.speedValue = mLatestObd2.speed();
			mLastSentCamInfo.hasOBD2 = true;
			mLastSentCamInfo.lastObd2 = obd2Package::OBD2(mLatestObd2); //data needs to be copied to a new buffer because new obd2 data can be received before sending
		} else {
			mLastSentCamInfo.hasOBD2 = false;
			cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.speed.speedValue = SpeedValue_unavailable;
		}
		cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.speed.speedConfidence = SpeedConfidence_unavailable;
		mMutexLatestObd2.unlock();

		cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.vehicleLength.vehicleLengthValue = VehicleLengthValue_unavailable;
		cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.vehicleLength.vehicleLengthConfidenceIndication = VehicleLengthConfidenceIndication_unavailable;

		cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.vehicleWidth = VehicleWidth_unavailable;

		cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.yawRate.yawRateValue = YawRateValue_unavailable;
		cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.yawRate.yawRateConfidence = YawRateConfidence_unavailable;

	}


	// Optional
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

	// Printing the cam structure
	//	asn_fprint(stdout, &asn_DEF_CAM, cam);


    //TODO: Free the allocated structure for cam. Is this enough?
	//asn_DEF_CAM.free_struct(&asn_DEF_CAM, cam, 0);
    return cam;
}

camPackage::CAM CaService::convertAsn1toProtoBuf(CAM_t* cam) {
	camPackage::CAM camProto;
	// header
	its::ItsPduHeader* header = new its::ItsPduHeader;
	header->set_messageid(cam->header.messageID);
	header->set_protocolversion(cam->header.protocolVersion);
	header->set_stationid(cam->header.stationID);
	camProto.set_allocated_header(header);

	// coop awareness
	its::CoopAwareness* coop = new its::CoopAwareness;
	coop->set_gendeltatime(cam->cam.generationDeltaTime);
	its::CamParameters* params = new its::CamParameters;

	// basic container
	its::BasicContainer* basicContainer = new its::BasicContainer;

	basicContainer->set_stationtype(cam->cam.camParameters.basicContainer.stationType);
	basicContainer->set_latitude(cam->cam.camParameters.basicContainer.referencePosition.latitude);
	basicContainer->set_longitude(cam->cam.camParameters.basicContainer.referencePosition.longitude);
	basicContainer->set_altitude(cam->cam.camParameters.basicContainer.referencePosition.altitude.altitudeValue);
	basicContainer->set_altitudeconfidence(cam->cam.camParameters.basicContainer.referencePosition.altitude.altitudeConfidence);
	basicContainer->set_semimajorconfidence(cam->cam.camParameters.basicContainer.referencePosition.positionConfidenceEllipse.semiMajorConfidence);
	basicContainer->set_semiminorconfidence(cam->cam.camParameters.basicContainer.referencePosition.positionConfidenceEllipse.semiMinorConfidence);
	basicContainer->set_semimajororientation(cam->cam.camParameters.basicContainer.referencePosition.positionConfidenceEllipse.semiMajorOrientation);
	params->set_allocated_basiccontainer(basicContainer);

	// high frequency container
	its::HighFreqContainer* highFreqContainer = new its::HighFreqContainer;
	its::BasicVehicleHighFreqContainer* basicHighFreqContainer = 0;
	its::RsuHighFreqContainer* rsuHighFreqContainer = 0;
	switch (cam->cam.camParameters.highFrequencyContainer.present) {
		case HighFrequencyContainer_PR_basicVehicleContainerHighFrequency:
			highFreqContainer->set_type(its::HighFreqContainer_Type_BASIC_HIGH_FREQ_CONTAINER);
			basicHighFreqContainer = new its::BasicVehicleHighFreqContainer();
			basicHighFreqContainer->set_heading(cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.heading.headingValue);
			basicHighFreqContainer->set_headingconfidence(cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.heading.headingConfidence);
			basicHighFreqContainer->set_speed(cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.speed.speedValue);
			basicHighFreqContainer->set_speedconfidence(cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.speed.speedConfidence);
			basicHighFreqContainer->set_drivedirection(cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.driveDirection);
			basicHighFreqContainer->set_vehiclelength(cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.vehicleLength.vehicleLengthValue);
			basicHighFreqContainer->set_vehiclelengthconfidence(cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.vehicleLength.vehicleLengthConfidenceIndication);
			basicHighFreqContainer->set_vehiclewidth(cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.vehicleWidth);
			basicHighFreqContainer->set_longitudinalacceleration(cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.longitudinalAcceleration.longitudinalAccelerationValue);
			basicHighFreqContainer->set_longitudinalaccelerationconfidence(cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.longitudinalAcceleration.longitudinalAccelerationValue);
			basicHighFreqContainer->set_curvature(cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.curvature.curvatureValue);
			basicHighFreqContainer->set_curvatureconfidence(cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.curvature.curvatureConfidence);
			basicHighFreqContainer->set_curvaturecalcmode(cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.curvatureCalculationMode);
			basicHighFreqContainer->set_yawrate(cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.yawRate.yawRateValue);
			basicHighFreqContainer->set_yawrateconfidence(cam->cam.camParameters.highFrequencyContainer.choice.basicVehicleContainerHighFrequency.yawRate.yawRateConfidence);

			// optional fields
			//basicHighFreqContainer->set_accelerationcontrol();
			//basicHighFreqContainer->set_laneposition();
			//basicHighFreqContainer->set_steeringwheelangle();
			//basicHighFreqContainer->set_steeringwheelangleconfidence();
			//basicHighFreqContainer->set_lateralacceleration();
			//basicHighFreqContainer->set_lateralaccelerationconfidence();
			//basicHighFreqContainer->set_verticalacceleration();
			//basicHighFreqContainer->set_verticalaccelerationconfidence();
			//basicHighFreqContainer->set_performanceclass();
			//basicHighFreqContainer->set_protectedzonelatitude();
			//basicHighFreqContainer->set_has_protectedzonelongitude();
			//basicHighFreqContainer->set_cendsrctollingzoneid();

			highFreqContainer->set_allocated_basicvehiclehighfreqcontainer(basicHighFreqContainer);
			break;

		case HighFrequencyContainer_PR_rsuContainerHighFrequency:
			highFreqContainer->set_type(its::HighFreqContainer_Type_RSU_HIGH_FREQ_CONTAINER);

			rsuHighFreqContainer = new its::RsuHighFreqContainer();
			// optional fields
			//rsuHighFreqContainer->

			highFreqContainer->set_allocated_rsuhighfreqcontainer(rsuHighFreqContainer);
			break;

		default:
			break;
	}
	params->set_allocated_highfreqcontainer(highFreqContainer);

	// low frequency container (optional)
	if(!cam->cam.camParameters.lowFrequencyContainer) {
		// fill in the low freq container
	}

	// special vehicle container (optional)
	if(!cam->cam.camParameters.specialVehicleContainer) {
		// fill in the special vehicle container
	}

	coop->set_allocated_camparameters(params);
	camProto.set_allocated_coop(coop);

	return camProto;
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
