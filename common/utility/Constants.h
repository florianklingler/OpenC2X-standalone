/*
 * Constants.h
 *
 *  Created on: Apr 21, 2016
 *      Author: root
 */

#ifndef UTILITY_CONSTANTS_H_
#define UTILITY_CONSTANTS_H_


//ignore unused function warning
#pragma GCC diagnostic ignored "-Wunused-variable"

#include <cstdint>

//ethertype for ethernet header used in wifi commuication
static const uint16_t ETHERTYPE_CAR = 0x0CA4;

static const uint16_t PRIORITY_VO =6;
static const uint16_t PRIORITY_VI =4;
static const uint16_t PRIORITY_BE =0;
static const uint16_t PRIORITY_BK =1;

#endif /* UTILITY_CONSTANTS_H_ */
