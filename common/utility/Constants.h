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


#ifndef UTILITY_CONSTANTS_H_
#define UTILITY_CONSTANTS_H_

/**
 * @addtogroup common
 * @{
 * 		@addtogroup constants Constants
 * 		Constants used by multiple Modules.
 * 		@{
 */

//ignore unused function warning
#pragma GCC diagnostic ignored "-Wunused-variable"

#include <cstdint>
#include <string>

/**ethertype for GeoNetworking*/
static const uint16_t ETHERTYPE_CAR = 0x8947;

/** Corresponding Hardware queue number for Priority */
static const int PRIORITY_VO =6;
/** Corresponding Hardware queue number for Priority */
static const int PRIORITY_VI =4;
/** Corresponding Hardware queue number for Priority */
static const int PRIORITY_BE =0;
/** Corresponding Hardware queue number for Priority */
static const int PRIORITY_BK =1;

/** config name for DCC **/
static const std::string DCC_CONFIG_NAME = "dcc";
/** config name for CAM **/
static const std::string CAM_CONFIG_NAME = "cam";
/** config name for DENM **/
static const std::string DENM_CONFIG_NAME = "denm";
/** config name for GPS **/
static const std::string GPS_CONFIG_NAME = "gps";
/** config name for OBD2 **/
static const std::string OBD2_CONFIG_NAME = "obd2";
/** config name for ldm **/
static const std::string LDM_CONFIG_NAME = "ldm";
/** config name for http server **/
static const std::string HTTP_SERVER_CONFIG_NAME = "httpServer";

/** module name for DCC **/
static const std::string DCC_MODULE_NAME = "Dcc";
/** module name for Channel Prober **/
static const std::string CHANNEL_PROBER_MODULE_NAME = "ChannelProber";
/** module name for Channel Prober **/
static const std::string PKT_STATS_COLLECTOR_MODULE_NAME = "PktStatsCollector";

/** module name for CAM **/
static const std::string CAM_MODULE_NAME = "CaService";

/** module name for DENM **/
static const std::string DENM_MODULE_NAME = "DenService";

/** module name for DENM App **/
static const std::string DENM_APP_MODULE_NAME = "DenmApp";

/** module name for GPS **/
static const std::string GPS_MODULE_NAME = "GPS";

/** module name for OBD2 **/
static const std::string OBD2_MODULE_NAME = "Obd2Service";

/** module name for LDM **/
static const std::string LDM_MODULE_NAME = "Ldm";

/** module name for http server **/
static const std::string HTTP_SERVER_MODULE_NAME = "WebApplication";



/**
 * @}
 */

/**
 * @}
 */

#endif /* UTILITY_CONSTANTS_H_ */
