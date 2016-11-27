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

#include "denservice.h"
#include <google/protobuf/text_format.h>
#include <unistd.h>
#include <iostream>
#include <ctime>
#include <chrono>
#include <string>
#include <stdlib.h>
#include <common/utility/Utils.h>
#include <common/asn1/TimestampIts.h>

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

	mMsgUtils = new MessageUtils("DenService", mGlobalConfig.mExpNo);
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

	delete mMsgUtils;
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
	string serializedAsnDenm;
	string serializedProtoDenm;

	while(1) {
		pair<string, string> received = mReceiverFromDcc->receive();
		envelope = received.first;
		serializedAsnDenm = received.second;

		DENM_t* denm = 0;
		int res = mMsgUtils->decodeMessage(&asn_DEF_DENM, (void **)&denm, serializedAsnDenm);
		if (res != 0) {
			mLogger->logError("Failed to decode received DENM. Error code: " + to_string(res));
			continue;
		}
//		asn_fprint(stdout, &asn_DEF_DENM, denm);
		denmPackage::DENM denmProto = convertAsn1toProtoBuf(denm);
		denmProto.SerializeToString(&serializedProtoDenm);

		mLogger->logInfo("forward incoming DENM " + to_string(denm->header.stationID) + " to LDM");
		mSenderToLdm->send(envelope, serializedProtoDenm);
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
//	denmPackage::DENM denm;
//	denm.ParseFromString(serializedDenm);
//	int64_t createTime = denm.createtime();
//	int64_t receiveTime = Utils::currentTime();
//	mLogger->logStats(denm.stationid() + "\t" + to_string(denm.id()) + "\t" + Utils::readableTime(createTime) + "\t" + Utils::readableTime(receiveTime));
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

//generate DENM and send to LDM and DCC
void DenService::send(triggerPackage::TRIGGER trigger) {
	string serializedData;
	dataPackage::DATA data;

	// create denm
	DENM_t* denm = generateDenm2();
	// asn_fprint(stdout, &asn_DEF_DENM, denm);
	vector<uint8_t> encodedDenm = mMsgUtils->encodeMessage(&asn_DEF_DENM, denm);
	string strDenm(encodedDenm.begin(), encodedDenm.end());
	mLogger->logDebug("Encoded DENM size: " + to_string(strDenm.length()));

	data.set_id(messageID_denm);
	data.set_type(dataPackage::DATA_Type_DENM);
	data.set_priority(dataPackage::DATA_Priority_VI);

	int64_t currTime = Utils::currentTime();
	data.set_createtime(currTime);
	data.set_validuntil(currTime + 2*1000*1000*1000);	//2s TODO: conform to standard? -> specify using CLI
	data.set_content(strDenm);
	data.SerializeToString(&serializedData);
	mLogger->logInfo("send new DENM " + to_string(data.id()) + " to DCC and LDM");
	mSenderToDcc->send("DENM", serializedData);		//send serialized DATA to DCC

	denmPackage::DENM denmProto = convertAsn1toProtoBuf(denm);
	string serializedProtoDenm;
	denmProto.SerializeToString(&serializedProtoDenm);
	mSenderToLdm->send("DENM", serializedProtoDenm);		//send serialized DENM to LDM
	//asn_DEF_DENM.free_struct(&asn_DEF_DENM, denm, 0);
}

//generate new DENM with increasing ID and current timestamp
denmPackage::DENM DenService::generateDenm(triggerPackage::TRIGGER trigger) {
	denmPackage::DENM denm;

//	//create DENM
//	denm.set_stationid(mGlobalConfig.mMac);
//	denm.set_id(mIdCounter++);
//	denm.set_content(trigger.content());
//	denm.set_createtime(Utils::currentTime());
//
//	mMutexLatestGps.lock();
//	if(mLatestGps.has_time()) {											//only add gps if valid data is available
//		gpsPackage::GPS* gps = new gpsPackage::GPS(mLatestGps);			//data needs to be copied to a new buffer because new gps data can be received before sending
//		denm.set_allocated_gps(gps);
//	}
//	mMutexLatestGps.unlock();
//
//	mMutexLatestObd2.lock();
//	if(mLatestObd2.has_time()) {										//only add obd2 if valid data is available
//		obd2Package::OBD2* obd2 = new obd2Package::OBD2(mLatestObd2);	//data needs to be copied to a new buffer because new obd2 data can be received before sending
//		denm.set_allocated_obd2(obd2);
//		//TODO: delete obd2, gps?
//	}
//	mMutexLatestObd2.unlock();

	return denm;
}

DENM_t* DenService::generateDenm2() {
	DENM_t* denm = static_cast<DENM_t*>(calloc(1, sizeof(DENM_t)));
	if (!denm) {
		throw runtime_error("could not allocate DENM_t");
	}
	// ITS pdu header
	denm->header.stationID = mGlobalConfig.mStationID;// mIdCounter; //
	denm->header.messageID = messageID_denm;
	denm->header.protocolVersion = protocolVersion_currentVersion;

	denm->denm.management.actionID.originatingStationID = mGlobalConfig.mStationID;
	denm->denm.management.actionID.sequenceNumber = 1;
	int64_t currTime = Utils::currentTime();
	cout << "currnet time : " << currTime << endl;

	// Seems like ASN1 supports 32 bit int (strange) and timestamp needs 42 bits.
	TimestampIts_t* timestamp = static_cast<TimestampIts_t*>(calloc(1, sizeof(TimestampIts_t)));
	timestamp->buf = static_cast<uint8_t*>(calloc(6, 1)); // timestamp needs 42 bits in standard
	timestamp->bits_unused = 6;
	timestamp->size = 6;
	memcpy(timestamp->buf, &currTime, 6);

	denm->denm.management.detectionTime = *timestamp;
	denm->denm.management.referenceTime = *timestamp;
	denm->denm.management.stationType = StationType_passengerCar;
	mMutexLatestGps.lock();
	if(mLatestGps.has_time()) {	//only add gps if valid data is available
		denm->denm.management.eventPosition.latitude = mLatestGps.latitude() * 10000000; // in one-tenth of microdegrees
		denm->denm.management.eventPosition.longitude = mLatestGps.longitude() * 10000000; // in one-tenth of microdegrees
		denm->denm.management.eventPosition.altitude.altitudeValue = mLatestGps.altitude();
	} else {
		denm->denm.management.eventPosition.latitude = Latitude_unavailable;
		denm->denm.management.eventPosition.longitude = Longitude_unavailable;
		denm->denm.management.eventPosition.altitude.altitudeValue = AltitudeValue_unavailable;
	}
	mMutexLatestGps.unlock();
	denm->denm.management.eventPosition.altitude.altitudeConfidence = AltitudeConfidence_unavailable;
	denm->denm.management.eventPosition.positionConfidenceEllipse.semiMajorConfidence = SemiAxisLength_unavailable;
	denm->denm.management.eventPosition.positionConfidenceEllipse.semiMajorOrientation = HeadingValue_unavailable;
	denm->denm.management.eventPosition.positionConfidenceEllipse.semiMinorConfidence = SemiAxisLength_unavailable;
	denm->denm.management.validityDuration = static_cast<long*>(calloc(sizeof(long), 1));
	*denm->denm.management.validityDuration = 60;
	return denm;
}

denmPackage::DENM DenService::convertAsn1toProtoBuf(DENM_t* denm) {
	denmPackage::DENM denmProto;

	// header
	its::ItsPduHeader* header = new its::ItsPduHeader;
	header->set_messageid(denm->header.messageID);
	header->set_protocolversion(denm->header.protocolVersion);
	header->set_stationid(denm->header.stationID);
	denmProto.set_allocated_header(header);

	// DENM containers
	its::DENMessage* denmMsg = new its::DENMessage;
	its::DENMManagementContainer* mgtCtr = new its::DENMManagementContainer;
	mgtCtr->set_stationid(denm->denm.management.actionID.originatingStationID);
	mgtCtr->set_sequencenumber(denm->denm.management.actionID.sequenceNumber);
//	mgtCtr->set_detectiontime(denm->denm.management.detectionTime);
//	mgtCtr->set_referencetime(denm->denm.management.referenceTime);
	mgtCtr->set_latitude(denm->denm.management.eventPosition.latitude);
	mgtCtr->set_longitude(denm->denm.management.eventPosition.longitude);
	mgtCtr->set_semimajorconfidence(denm->denm.management.eventPosition.positionConfidenceEllipse.semiMajorConfidence);
	mgtCtr->set_semiminorconfidence(denm->denm.management.eventPosition.positionConfidenceEllipse.semiMinorConfidence);
	mgtCtr->set_semimajororientation(denm->denm.management.eventPosition.positionConfidenceEllipse.semiMajorOrientation);
	mgtCtr->set_altitude(denm->denm.management.eventPosition.altitude.altitudeValue);
	mgtCtr->set_altitudeconfidence(denm->denm.management.eventPosition.altitude.altitudeConfidence);
	if(!denm->denm.management.validityDuration) {
		mgtCtr->set_validityduration(*denm->denm.management.validityDuration);
	}
	mgtCtr->set_stationtype(denm->denm.management.stationType);
	denmMsg->set_allocated_managementcontainer(mgtCtr);
	denmProto.set_allocated_msg(denmMsg);
	return denmProto;
}

dataPackage::DATA DenService::generateData(denmPackage::DENM denm) {
	dataPackage::DATA data;
//	string serializedDenm;

//	//serialize DENM
//	denm.SerializeToString(&serializedDenm);
//
//	//create DATA
//	data.set_id(denm.id());
//	data.set_type(dataPackage::DATA_Type_DENM);
//	data.set_priority(dataPackage::DATA_Priority_VI);
//
//	data.set_createtime(denm.createtime());
//	data.set_validuntil(denm.createtime() + 1*1000*1000*1000);	//1s TODO: conform to standard? -> specify using CLI
//	data.set_content(serializedDenm);

	return data;
}

int main() {
	DenService denm;
	denm.init();

	return EXIT_SUCCESS;
}
