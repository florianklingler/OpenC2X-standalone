/*
 * Constants.h
 *
 *  Created on: Apr 21, 2016
 *      Author: root
 */

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
