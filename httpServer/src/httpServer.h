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


#ifndef HTTPSERVER_H_
#define HTTPSERVER_H_

/**
 * @addtogroup httpServer
 * @{
 */

#include <common/config/config.h>
#include <common/utility/CommunicationClient.h>
#include <common/utility/LoggingUtility.h>
#include <common/utility/Constants.h>

#include <common/buffers/cam.pb.h>
#include <common/buffers/denm.pb.h>
#include <common/buffers/gps.pb.h>
#include <common/buffers/obd2.pb.h>
#include <common/buffers/dccInfo.pb.h>
#include <common/buffers/camInfo.pb.h>
#include <common/buffers/ldmData.pb.h>
#include <google/protobuf/text_format.h>
#include <mutex>
#include <ctime>
#include <iostream>

/** Struct that hold the configuration for httpServer.
 * The configuration is defined in /etc/config/openc2x_httpServer
 */
struct httpServerConfig {
	int mTimeout;

	void loadConfig() {
		ptree pt = load_config_tree();
		mTimeout = pt.get("httpServer.timeout", 100);
	}
};

/** A Server that connects to LDMs via ZMQ and exposes it's Data via http.
 * Uses the Crow Framework (<a href="https://github.com/ipkn/crow"> git link</a>) for the http service.
 * The received data/messages from LDM is serialized as Protobuffer and converted to JSON with the library pbjson.
 * In addition to the serialized messages, the JSON string also includes the type and the number of the messages.
 */
class httpServer {
public:

	httpServer(GlobalConfig config);
	virtual ~httpServer();

	/** Gets CAMs from LDM and converts them to JSON.
	 * Requests CAMs from LDM via ZMQ and converts them from Protobuffer to JSON.
	 *
	 * Condition is not working at the moment. Always returns latest CAM from each known Station ID.
	 * @param condition if =="latest" returns only the latest Cams else returns all Cams
	 * @return JSON String of a Array of CAM(s)
	 */
	std::string requestCam(std::string condition);

	/** Gets DENMs from LDM and converts them to JSON.
	 * Requests DENMs from LDM via ZMQ and converts them from Protobuffer to JSON.
	 *
	 * Condition is not working at the moment. Always returns latest DENMs from each known Station ID.
	 * @param condition if =="latest" returns only the latest DENMs else returns all DENMs
	 * @return JSON String of a Array of DENM(s)
	 */
	std::string requestDenm(std::string condition);

	/** unused.
	 * @todo check whether function is needed/working
	 * @deprecated not tested
	 * @param condition
	 * @return
	 */
	std::string requestGps(std::string condition);

	/** unused.
	 * @todo check whether function is needed/working
	 * @deprecated not tested
	 * @param condition
	 * @return
	 */
	std::string requestObd2(std::string condition);

	/** Gets a JSON String containing information about the local DCC from LDM.
	 * Returns a JSON String which encodes a array containing four information objects
	 * about the four access categorys of DCC from LDM via ZMQ.
	 *
	 * Condition is not working at the moment.
	 * @param condition condition if =="latest" returns only the latest else return all
	 * @return JSON String of DCC Status Information
	 */
	std::string requestDccInfo(std::string condition);

	/** Gets a JSON String containing Information about the last created
	 * CAM by the local CAM Service from LDM.
	 * Contains Information like triggering reason oder creation time. Obtained from LDM via ZMQ.
	 *
	 * @param condition
	 * @return JSON String of Information about the most recent generated CAM
	 */
	std::string requestCamInfo(std::string condition);
	
	/** Logs a given string to a logfile
	 *
	 * @param message
	 * @return Empty string or error code
	 */
	std::string logMessage(std::string message);

	/**returns own mac address of the used ethernet device defined in the global config.
	 * @return String of MAC address
	 */
	std::string myMac();

private:
	GlobalConfig mGlobalConfig;
	httpServerConfig mLocalConfig;

	CommunicationClient* mClientCam;
	CommunicationClient* mClientDenm;
	CommunicationClient* mClientGps;
	CommunicationClient* mClientObd2;
	CommunicationClient* mClientCamInfo;
	CommunicationClient* mClientDccInfo;

	std::mutex mMutexCam;
	std::mutex mMutexDenm;
	std::mutex mMutexGps;
	std::mutex mMutexObd2;
	std::mutex mMutexCamInfo;
	std::mutex mMutexDccInfo;

	LoggingUtility* mLogger;
};
/** @}*/ //end group
#endif /* HTTPSERVER_H_ */
