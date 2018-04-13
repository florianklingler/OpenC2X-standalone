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


#include "MessageUtils.h"
#include <common/asn1/per_encoder.h>
#include <common/asn1/per_decoder.h>
#include <iterator>

using namespace std;
using boost::property_tree::ptree;


MessageUtils::MessageUtils(LoggingUtility& logger) : mLogger(logger) {
	
}

int MessageUtils::writeOut(const void *buffer, size_t size, void *app_key) {
    vector<uint8_t>* payload = static_cast<vector<uint8_t>*>(app_key);
    // memcpy(&message, buffer, size);
    // http://en.cppreference.com/w/cpp/algorithm/copy_n
    copy_n(static_cast<const uint8_t*>(buffer), size, std::back_inserter(*payload));
    return 0;
}

vector<uint8_t> MessageUtils::encodeMessage(asn_TYPE_descriptor_t *td, void *structPtr) {
	vector<uint8_t> payload;
	asn_enc_rval_t erv = uper_encode(td, const_cast<void*>(structPtr), &MessageUtils::writeOut, &payload);
	mLogger.logInfo("Encoded bytes: " + to_string(erv.encoded));
	if(erv.encoded == -1) {
		throw runtime_error("Encoding failed");
	}
	return payload;
}

int MessageUtils::decodeMessage(asn_TYPE_descriptor_t *td, void** t, string buffer) {
	asn_codec_ctx_t ctx{};
	asn_dec_rval_t drv = uper_decode_complete(&ctx, td, t, buffer.data(), buffer.length());
	//asn_dec_rval_t drv = uper_decode(&context, td, t, buffer.data(), buffer.length(), 0, 0);
	mLogger.logInfo("Decoded bytes: " + to_string(drv.consumed) + " and returning code: " + to_string(drv.code));
	return drv.code;// == RC_OK;
}
