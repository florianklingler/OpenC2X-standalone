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
// Christoph Sommer <sommer@ccs-labs.org>


#ifndef GPSSERVICE_H_
#define GPSSERVICE_H_

/**
 * @addtogroup gps
 * @{
 */
#include <gps.h>
#include <common/utility/CommunicationSender.h>
#include <common/utility/LoggingUtility.h>
#include <common/buffers/build/gps.pb.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/asio.hpp>
#include <common/config/config.h>
#include <fstream>

/** Struct that hold the configuration for GpsService.
 * The configuration is defined in <a href="../../gps/config/config.xml">gps/config/config.xml</a>
 */
struct GpsConfig {
	/**
	 * Specifies whether real or simulated GPS data should be used (default: true).
	 */
	bool mSimulateData;
	/**
	 * Path to recored GPS traces. Only needed if recored traces should be used.
	 */
	std::string mGpsDataFile;
	/**
	 * If GPS data is simulated, mode specifies how the GPS data is generated (default: 0).
	 * Mode 0 means, it is randomly generated (moving straight towards north).
	 * Mode 1 means, the recored GPS traces are used.
	 */
	int mMode;


	void loadConfigXML(const std::string &filename) {
		boost::property_tree::ptree pt;
		read_xml(filename, pt);

		mSimulateData = pt.get("gps.SimulateData", true);
		mGpsDataFile = pt.get("gps.DataFile", "");
		mMode = pt.get("gps.SimulationMode", 0);
	}
};

typedef struct std::pair<double, double> position;

/**
 * Class that connects to the gps deamon and offers its data to the other modules via zmq.
 */
class GpsService {
public:
	GpsService(GpsConfig &config, std::string globalConfig, std::string loggingConf, std::string statisticConf);
	~GpsService();

	/** Connects to GPSd.
	 *
	 * @return true if connected successfully
	 */
	bool connectToGpsd();

	/** Writes measured GPS data in the specified gpsdata struct.
	 *
	 * @param gpsdata Struct to store the GPS data in
	 * @return 0 = success, -1 = error
	 */
	int getGpsData(struct gps_data_t* gpsdata);

	/** Converts the measured gpsdata struct to GPS protobuffer.
	 *
	 * @param gpsdata struct to convert
	 * @return resulting protobuffer
	 */
	gpsPackage::GPS gpsDataToBuffer(struct gps_data_t* gpsdata);

	/** Receives actual GPS data, logs and sends it to the services.
	 *
	 */
	void receiveData();

	/** Simulates realistic vehicle speed.
	 * Speed is between 0 and 15 m/s.
	 * @return Speed in m/s
	 */
	double simulateSpeed();

	/** Calculates new simulated position.
	 * New position = Start + offsetN/E in north/east direction
	 * @param start Start position
	 * @param offsetN Offset in north direction in meters
	 * @param offsetE Offset in east direction in meters
	 * @return New position
	 */
	position simulateNewPosition(position start, double offsetN, double offsetE);

	/** Periodically (every 100ms) simulates GPS data, logs and sends it to the services.
	 *
	 * @param ec Boost error code
	 * @param currentPosition Current position
	 */
	void simulateData(const boost::system::error_code &ec, position currentPosition);

	/** Reproduces GPS data from pre-recorded GPS trail.
	 *
	 * @param ec Boost error code
	 */
	void simulateFromDemoTrail(const boost::system::error_code &ec);

	/** Parses the recorded GPS data and converts it to a GPS protobuffer.
	 *
	 * @param data GPS data from recorded trail
	 * @return GPS protobuffer
	 */
	gpsPackage::GPS convertTrailDataToBuffer(std::string data);

	/** Sends the specified GPS protobuffer to the services (CaService and DenService) via zmq.
	 *
	 * @param gps GPS protobuffer to be sent
	 */
	void sendToServices(gpsPackage::GPS gps);

	static void closeGps();
	void startStreaming();
	static void stopStreaming();

	/** Initializes depending on the config.
	 * Based on the config, either connects to GPSd (for real GPS data) or initializes the simulation.
	 */
	void init();

private:
	GpsConfig mConfig;
	GlobalConfig mGlobalConfig;
	static struct gps_data_t mGpsData;
	/**
	 * Time of last received/measured GPS.
	 */
	double mLastTime;		//time of last received/measured GPS

	CommunicationSender* mSender;
	LoggingUtility* mLogger;

	//for simulation only
	std::default_random_engine mRandNumberGen;
	std::bernoulli_distribution mBernoulli;
	std::uniform_real_distribution<double> mUniform;

	boost::asio::io_service mIoService;
	boost::asio::deadline_timer* mTimer;

	/**
	 * File containing a recored GPS trace.
	 */
	std::ifstream mFile;
};

#endif


/** @} */ //end group
