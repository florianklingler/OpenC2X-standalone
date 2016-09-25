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
// Gurjashan Singh Pannu <gurjashan.pannu@ccs-labs.org>

#ifndef GEONETHEADERS_H_
#define GEONETHEADERS_H_

#include <net/ethernet.h>
#include <stdint.h>

struct GeoNetBasicHeader {
	uint8_t versionAndNH;
	uint8_t reserved;
	uint8_t lifetime;
	uint8_t remainingHopLimit;
};

struct GeoNetCommonHeader {
	uint8_t nhAndReserved;
	uint8_t htAndHst;
	uint8_t tc;
	uint8_t flags;
	uint16_t payload;
	uint8_t maxHop;
	uint8_t reserved;
};

struct GeoNetAddress {
	uint16_t assignmentTypeCountryCode;
	uint8_t llAddr[ETH_ALEN];
};

struct SourcePositionVector {
	GeoNetAddress addr;
	uint32_t timestamp;
	uint32_t latitude;
	uint32_t longitude;
	uint16_t speed;
	uint16_t heading;
};

struct GeoNetTSB {
	SourcePositionVector spv;
	uint32_t reserved;
};

struct GeoNetHeader {
	GeoNetBasicHeader basicHeader;
	GeoNetCommonHeader commonHeader;
	GeoNetTSB tsb;
};

struct BTPHeader {
	uint16_t mDestinationPort;
	uint16_t mSourcePort;
};

struct GeoNetworkAndBTPHeader {
	GeoNetHeader mGeoNetHdr;
	BTPHeader mBTPHdr;
};




#endif /* GEONETHEADERS_H_ */
