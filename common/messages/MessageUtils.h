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

#include <asn_application.h>
#include <string>
#include "../utility/LoggingUtility.h"

/**
 * Contains message utility functions (encoding/ decoding) used by other modules.
 *
 * @ingroup common
 */
class MessageUtils {
public:
	MessageUtils(std::string moduleName, int expNo);
	virtual ~MessageUtils();

	static int write_out(const void *buffer, size_t size, void *app_key);
	asn_enc_rval_t encodeMessage(struct asn_TYPE_descriptor_s *type_descriptor, void *struct_ptr);
	void decodeMessage();

	LoggingUtility* mLogger;
};

#endif /* UTILS_H_ */
