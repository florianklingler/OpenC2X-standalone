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


#ifndef UTILITY_COMMUNICATIONCLIENT_H_
#define UTILITY_COMMUNICATIONCLIENT_H_

#include <string>
#include <zmq.hpp>
#include "external/zhelpers.hpp"
#include "LoggingUtility.h"
#include <mutex>

/**
 * Send ZMQ request to CommunicationServer and waits a specified time for an answer.
 *
 * @ingroup communication
 */
class CommunicationClient {
public:
	/**
	 * ownerModule and expNo forwarded to LoggingUtility constructor
	 * @param ownerModule Module Name
	 * @param expNo Experiment Number
	 * @param portOut port used for requesting
	 */
	CommunicationClient(std::string portOut,  LoggingUtility& logger);
	~CommunicationClient();
	std::string sendRequest(std::string envelope, std::string request, int timeout);
	void init();

private:
	std::string mPortOut;

	zmq::context_t* mContext;
	zmq::socket_t* mClient;

	LoggingUtility& mLogger;

	std::mutex mMutex;
};

#endif
