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


#ifndef LDM_H_
#define LDM_H_

/**
 * @addtogroup ldm
 * @{
 */

#include <boost/thread.hpp>
#include <common/utility/CommunicationReceiver.h>
#include <common/utility/CommunicationServer.h>
#include <common/utility/LoggingUtility.h>
#include <sqlite3.h>
#include <common/buffers/build/cam.pb.h>
#include <common/buffers/build/denm.pb.h>
#include <common/buffers/build/gps.pb.h>
#include <common/buffers/build/obd2.pb.h>
#include <common/buffers/build/dccInfo.pb.h>
#include <common/buffers/build/camInfo.pb.h>
#include <common/buffers/build/ldmData.pb.h>
#include <google/protobuf/text_format.h>
#include <string>
#include <ctime>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <mutex>

/**
 * The Local Dynamic Map (LDM) is responsible for maintaining data that is part of ITS.
 *
 * The data is written into a SQLite database. In addition, the latest data is cached in order to quickly answer requests.
 * (Apparently, sql queries are too slow to answer requests with a high frequency.)
 *
 * @nonStandard deletion of old data. for debugging add "deleted" flag and automatically "delete" entries that are eg. too old, too far away; only return non-delted entries
 *
 * @nonStandard no security checks for requests
 *
 * @nonStandard no registration of dataProviders, dataReceivers
 *
 * @nonStandard no way of general data requests, only latest data available
 */
class LDM {
public:
	LDM(std::string globalConfig, std::string loggingConf, std::string statisticConf);
	~LDM();
	void init();

	/** Creates the database schema.
	 *	Creates tables for storing CAM, DENM, GPS, OBD2, DccInfo and CamInfo if they do not exist already
	 */
	void createTables();

	/** Queries GPS from the database.
	 * 	Executes a SELECT query with a specified condition on the GPS table and returns the result.
	 * @param condition Some condition for restricting the returned data (e.g. a WHERE clause).
	 * @return A ldmData package containing the queried data.
	 */
	dataPackage::LdmData gpsSelect(std::string condition);	//TODO: only return latest GPS? useful?

	/** Queries OBD2 from the database.
	 * 	Executes a SELECT query with a specified condition on the OBD2 table and returns the result.
	 * @param condition Some condition for restricting the returned data (e.g. a WHERE clause).
	 * @return A ldmData package containing the queried data.
	 */
	dataPackage::LdmData obd2Select(std::string condition);

	/** Queries CAM from the database.
	 * 	Executes a SELECT query with a specified condition on the CAM table and returns the result.
	 * @param condition Some condition for restricting the returned data (e.g. a WHERE clause).
	 * @return A ldmData package containing the queried data.
	 */
	dataPackage::LdmData camSelect(std::string condition);

	/** Queries DENM from the database.
	 * 	Executes a SELECT query with a specified condition on the DENM table and returns the result.
	 * 	@param condition Some condition for restricting the returned data (e.g. a WHERE clause).
	 * 	@return A ldmData package containing the queried data.
	 */
	dataPackage::LdmData denmSelect(std::string condition);

	/** Queries DccInfo from the database.
	 * 	Executes a SELECT query with a specified condition on the DccInfo table and returns the result.
	 *	@param condition Some condition for restricting the returned data (e.g. a WHERE clause).
	 * 	@return A ldmData package containing the queried data.
	 */
	dataPackage::LdmData dccInfoSelect(std::string condition);

	/** Queries CamInfo from the database.
	 * 	Executes a SELECT query with a specified condition on the CamInfo table and returns the result.
	 * 	@param condition Some condition for restricting the returned data (e.g. a WHERE clause).
	 * 	@return A ldmData package containing the queried data.
	 */
	dataPackage::LdmData camInfoSelect(std::string condition);

	/**	Inserts arbitrary data into the database.
	 * 	Executes the specified INSERT command on the database.
	 * 	@param sqlCommand The SQL insert command to be executed.
	 */
	void insert(std::string sqlCommand);

	/**	Inserts CAM into the database.
	 * 	Constructs and executes queries for inserting the specified CAM data
	 * 	and if available also included GPS and OBD2 data into the corresponding tables.
	 *	@param cam The CAM to be inserted.
	 */
	void insertCam(camPackage::CAM cam);

	/**	Inserts DENM into the database.
	 * 	Constructs and executes queries for inserting the specified DENM data
	 * 	and if available also included GPS and OBD2 data into the corresponding tables.
	 *	@param denm The DENM to be inserted.
	 */
	void insertDenm(denmPackage::DENM denm);

	/**	Logs GPS.
	 * 	Constructs a human-readable representation of the specified GPS and logs it.
	 *	@param gps The GPS to be logged.
	 */
	void printGps(gpsPackage::GPS gps);

	/**	Logs OBD2.
	 * 	Constructs a human-readable representation of the specified OBD2 and logs it.
	 *	@param obd2 The OBD2 to be logged.
	 */
	void printObd2(obd2Package::OBD2 obd2);

	/**	Logs CAM.
	 * 	Constructs a human-readable representation of the specified CAM with included GPS and OBD2 if available and logs it.
	 *	@param cam The CAM to be logged.
	 */
	void printCam(camPackage::CAM cam);

	/**	Logs DENM.
	 * 	Constructs a human-readable representation of the specified DENM with included GPS and OBD2 if available and logs it.
	 *	@param denm The DENM to be logged.
	 */
	void printDenm(denmPackage::DENM denm);

	/**	Receives CAM.
	 *  Receives CAM from the local or a remote machine, caches the data and inserts it into the database.
	 */
	void receiveFromCa();

	/**	Receives DENM.
	 *  Receives DENM from the local or a remote machine, caches the data and inserts it into the database.
	 */
	void receiveFromDen();

	/**	Receives and answers requests.
	 *  Receives requests for querying data from a specified table, calls the corresponding select and finally replies to the request.
	 */
	void receiveRequest();

	/**	Receives DccInfo.
	 *  Receives DccInfo from the local machine, caches the data and inserts it into the database.
	 */
	void receiveDccInfo();

	/**	Receives CamInfo.
	 *  Receives CamInfo from the local machine, caches the data and inserts it into the database.
	 */
	void receiveCamInfo();

private:
	CommunicationReceiver* mReceiverFromDen;
	CommunicationReceiver* mReceiverFromCa;
	CommunicationReceiver* mReceiverDccInfo;
	CommunicationReceiver* mReceiverCamInfo;
	CommunicationServer* mServer;

	boost::thread* mThreadReceiveFromCa;
	boost::thread* mThreadReceiveFromDen;
	boost::thread* mThreadReceiveDccInfo;
	boost::thread* mThreadReceiveCamInfo;
	boost::thread* mThreadServer;

	LoggingUtility* mLogger;

	std::mutex mCamMutex;
	std::mutex mCamInfoMutex;
	std::mutex mDccInfoMutex;
	std::mutex mGpsMutex;
	std::mutex mObd2Mutex;
	std::mutex mDenmMutex;

	sqlite3* mDb;

	/**
	 * Cache for storing the latest CAM for each stationId
	 */
	std::map<std::string,camPackage::CAM> camCache;

	/**
	 * Cache for storing the latest CamInfo
	 */
	infoPackage::CamInfo camInfoCache;

	/**
	 * Cache for storing the latest DccInfo for each access category
	 */
	std::map<std::string,infoPackage::DccInfo>  dccInfoCache;

	/**
	 * Cache for storing the latest DENM for each stationId
	 */
	std::map<std::string,denmPackage::DENM> denmCache;
};

/** @} */ //end group
#endif
