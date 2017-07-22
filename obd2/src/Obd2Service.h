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


#ifndef OBD2SERVICE_H_
#define OBD2SERVICE_H_

/**
 * @addtogroup obd2
 * @{
 */

#include "SerialPort.h"
#include <common/utility/CommunicationSender.h>
#include <common/utility/LoggingUtility.h>
#include <common/buffers/build/obd2.pb.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/asio.hpp>
#include <string>
#include <common/config/config.h>

/** Struct that holds the configuration for Obd2Service.
 * The configuration is defined in <a href="../../obd2/config/config.xml">obd2/config/config.xml</a>.
 */
struct Obd2Config {
	/** True iff OBD2 should be simulated, false for using real data.
	 *
	 */
	bool mSimulateData;

	/** The USB device OBD2 is connected to (default: /dev/ttyUSB0).
	 *
	 */
	char* mDevice;

	/** The frequency in ms at which new OBD2 data should be distributed (default: 500).
	 *	Note: a frequency of 100ms does not work.
	 */
	int mFrequency;

	void loadConfigXML(const std::string &filename) {
		boost::property_tree::ptree pt;
		read_xml(filename, pt);

		mSimulateData = pt.get("obd2.SimulateData", true);
		std::string device = pt.get("obd2.Device", "//dev//ttyUSB0");
		mDevice = strdup(device.c_str());
		mFrequency = pt.get("obd2.Frequency", 100);
	}
};

/**
 * Class that connects to OBD2 via serial port and offers its data to other modules via ZMQ.
 */
class Obd2Service {
public:
	Obd2Service(Obd2Config &config, std::string globalConfig, std::string loggingConf, std::string statisticConf);
	~Obd2Service();
	void init();

	/** Reads the actual vehicle data from OBD2 and distributes it via ZMQ.
	 *  Periodically reads speed and rpm data from OBD2 if simulating data is turned off,
	 *	writes it into a protocol buffer and sends it to services such as CaService and DenService.
	 *	@param ec Boost error code.
	 *	@param serial SerialPort that handles the actual connection to OBD2 via serial port.
	 */
	void receiveData(const boost::system::error_code &ec, SerialPort* serial);

	/** Simulates vehicle speed.
	 *  Simulates vehicle speed using a random distribution.
	 */
	double simulateSpeed();

	/** Simulates vehicle data and distributes it via ZMQ.
	 *  Periodically simulates speed data if simulating data is turned on,
	 *	writes it into a protocol buffer and sends it to services such as CaService and DenService.
	 *	@param ec Boost error code.
	 */
	void simulateData(const boost::system::error_code &ec);

	/** Distributes OBD2 via ZMQ.
	 *  Sends OBD2 via ZMQ to services such as CaService and DenService and logs the data.
	 *  @param obd2 The OBD2 to be sent to services.
	 */
	void sendToServices(obd2Package::OBD2 obd2);

private:
	Obd2Config mConfig;
	GlobalConfig mGlobalConfig;

	CommunicationSender* mSender;
	LoggingUtility* mLogger;

	//for simulation only
	std::default_random_engine mRandNumberGen;
	std::bernoulli_distribution mBernoulli;
	std::uniform_real_distribution<double> mUniform;

	boost::asio::io_service mIoService;
	boost::asio::deadline_timer* mTimer;
};

/** @} */ //end group

#endif
