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

/**ethertype for ethernet header used in wifi commuication*/
static const uint16_t ETHERTYPE_CAR = 0x0CA4;

/** Corresponding Hardware queue number for Priority */
static const int PRIORITY_VO =6;
/** Corresponding Hardware queue number for Priority */
static const int PRIORITY_VI =4;
/** Corresponding Hardware queue number for Priority */
static const int PRIORITY_BE =0;
/** Corresponding Hardware queue number for Priority */
static const int PRIORITY_BK =1;

/**
 * @}
 */

/**
 * @}
 */

#endif /* UTILITY_CONSTANTS_H_ */
