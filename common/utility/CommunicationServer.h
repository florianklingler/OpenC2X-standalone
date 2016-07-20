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


#ifndef UTILITY_COMMUNICATIONSERVER_H_
#define UTILITY_COMMUNICATIONSERVER_H_

#include <string>
#include <zmq.hpp>
#include "external/zhelpers.hpp"
#include "LoggingUtility.h"

/**
 * Replys to ZMQ requests from CommunicationClient.
 *
 * @ingroup communication
 */
class CommunicationServer {
public:
	/**
	 * ownerModule and expNo forwarded to LoggingUtility constructor
	 * @param ownerModule Module Name
	 * @param expNo Experiment Number
	 * @param portOut port used for listening for requests and answering
	 */
	CommunicationServer(std::string ownerModule, std::string portOut, int expNo);
	~CommunicationServer();
	void sendReply(std::string reply);
	std::pair<std::string, std::string> receiveRequest();

private:
	std::string mOwnerModule;

	zmq::context_t* mContext;
	zmq::socket_t* mServer;

	LoggingUtility* mLogger;
};

#endif
