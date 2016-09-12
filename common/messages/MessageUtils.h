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


#ifndef MESSAGE_UTILS_H_
#define MESSAGE_UTILS_H_

#include "../utility/LoggingUtility.h"
#include "../asn1/asn_application.h"
#include <string>
#include <vector>

/**
 * Contains message utility functions (encoding/ decoding) used by other modules.
 *
 * @ingroup common
 */
class MessageUtils {
public:
	MessageUtils(std::string moduleName, int expNo);
	virtual ~MessageUtils();

	static int writeOut(const void *buffer, size_t size, void *app_key);
	std::vector<uint8_t> encodeMessage(asn_TYPE_descriptor_t *td, void *structPtr);
	int decodeMessage(asn_TYPE_descriptor_t* td, void** t, std::string buffer);

	LoggingUtility* mLogger;
};

#endif /* UTILS_H_ */
