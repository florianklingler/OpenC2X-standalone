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


#ifndef DENSERVICE_H_
#define DENSERVICE_H_

/**
 * @addtogroup denm
 * @{
 */

#include <boost/thread.hpp>
#include <config/config.h>
#include <utility/CommunicationReceiver.h>
#include <utility/CommunicationSender.h>
#include <utility/LoggingUtility.h>
#include <buffers/build/data.pb.h>
#include <buffers/build/denm.pb.h>
#include <buffers/build/gps.pb.h>
#include <buffers/build/obd2.pb.h>
#include <buffers/build/trigger.pb.h>
#include <mutex>

/**
 * Class that handles the receiving, creating and sending of DEN Messages.
 *
 * @nonStandard DENM repetition/keep alive protocol is not implemented!
 */
class DenService {
public:
	DenService();
	~DenService();

	/**
	 * Initializes DenService to receive DENMs from app (e.g. web interface)
	 */
	void init();

	/**
	 * Receives DENM from DCC and forwards it to LDM
	 */
	void receive();

	/** Logs the delay of the specified (serialized) DENM between its creation time and the current time.
	 * @param byteMessage senrialized DENM message
	 */
	void logDelay(std::string byteMessage);

	/**
	 * Triggers generation/ sending of DENM message by an external application.
	 */
	void triggerAppDenm();

	/**
	 * Sends a new DENM to LDM and DCC.
	 * @param trigger The data that the external application wants to include in the DENM.
	 */
	void send(triggerPackage::TRIGGER trigger);

	/**
	 * Generates a new DENM.
	 * @param trigger The data that the external application wants to include in the DENM.
	 * @return The newly generated Denm package.
	 */
	denmPackage::DENM generateDenm(triggerPackage::TRIGGER trigger);

	/** Generates a new data package that includes the specified DENM.
	 * The specified DENM is serialized and included in the data package.
	 * Additionally, the station id, priority, creation time of the DENM are included.
	 * @param denm The DENM to be included in the data package
	 * @return The newly generated data package.
	 */
	dataPackage::DATA generateData(denmPackage::DENM denm);

	/**
	 * Receives new GPS data from the GPS module.
	 */
	void receiveGpsData();

	/**
	 * Receives new OBD2 data from the OBD2 module.
	 */
	void receiveObd2Data();

private:
	GlobalConfig mGlobalConfig;

	CommunicationReceiver* mReceiverFromApp;
	CommunicationReceiver* mReceiverFromDcc;
	CommunicationSender* mSenderToDcc;
	CommunicationSender* mSenderToLdm;

	CommunicationReceiver* mReceiverGps;
	CommunicationReceiver* mReceiverObd2;

	boost::thread* mThreadReceive;
	boost::thread* mThreadGpsDataReceive;
	boost::thread* mThreadObd2DataReceive;
	boost::thread* mThreadAppTrigger;

	LoggingUtility* mLogger;

	/**
	 * Id for DENM message.
	 */
	long mIdCounter;

	gpsPackage::GPS mLatestGps;
	std::mutex mMutexLatestGps;

	obd2Package::OBD2 mLatestObd2;
	std::mutex mMutexLatestObd2;
};

/** @} */ //end group
#endif
