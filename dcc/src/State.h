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
// Florian Klingler <klingler@ccs-labs.org>


#ifndef STATE_H_
#define STATE_H_

/**
 * @addtogroup dcc
 * @{
 */
#include "DccConfig.h"
#include <map>

/**
 * Class that defines a state of DCC with characteristics such as DCC-mechanism, TX power, token interval, data rate, carrier sense, etc.
 */
class State {
public:
	size_t asStateId;
	double asChanLoad;

	DCC_NDL_Param<dcc_Mechanism_t> asDcc;
	DCC_NDL_Param<double> asTxPower;
	DCC_NDL_Param<double> asPacketInterval;	//token interval
	DCC_NDL_Param<double> asDatarate;
	DCC_NDL_Param<double> asCarrierSense;

	State(DCC_State_Config &cfg) {
		asStateId = cfg.asStateId;
		asChanLoad = cfg.asChanLoad;
		asDcc = DCC_NDL_Param<dcc_Mechanism_t>(cfg.asDcc);
		asTxPower = DCC_NDL_Param<double>(cfg.asTxPower);
		asPacketInterval = DCC_NDL_Param<double>(cfg.asPacketInterval);
		asDatarate = DCC_NDL_Param<double>(cfg.asDatarate);
		asCarrierSense = DCC_NDL_Param<double>(cfg.asCarrierSense);
	}

	~State() {
	}
};

typedef std::map<size_t, State> States;		//map of all states

/**
 * @}
 */
#endif /* STATE_H_ */
