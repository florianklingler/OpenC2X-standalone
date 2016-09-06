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

#include "MessageUtils.h"
#include "../asn1/per_encoder.h"
#include <iterator>

using namespace std;

MessageUtils::MessageUtils(string moduleName, int expNo) {
	mLogger = new LoggingUtility(moduleName, expNo);
}

MessageUtils::~MessageUtils() {}

int MessageUtils::write_out(const void *buffer, size_t size, void *app_key) {
    auto message = static_cast<vector<uint8_t>*>(app_key);
//    memcpy(&message, buffer, size);
    std::copy_n(static_cast<const uint8_t*>(buffer), size, std::back_inserter(*message));
    return 0;
}

asn_enc_rval_t MessageUtils::encodeMessage(struct asn_TYPE_descriptor_s *type_descriptor, void *struct_ptr) {
	asn_enc_rval_t erv;
	vector<uint8_t> message;
//	erv = uper_encode(type_descriptor, struct_ptr, write_out, stdout);
	erv = uper_encode(type_descriptor, const_cast<void*>(struct_ptr), &MessageUtils::write_out, &message);
	if(erv.encoded == -1) {
		stringstream ss;
		ss << "Could not encode " << erv.failed_type->name << " " << strerror(errno);
		mLogger->logError(ss.str());
		exit(1);
	}
	return erv;
}
