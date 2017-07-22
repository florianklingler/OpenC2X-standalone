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


#include "CommunicationServer.h"

using namespace std;

CommunicationServer::CommunicationServer(string ownerModule, string portOut, int expNo, string loggingConf, string statisticConf) {
	mOwnerModule = ownerModule;

	mContext = new zmq::context_t(1);
	mServer = new zmq::socket_t(*mContext, ZMQ_REP);
	mServer->bind(("tcp://*:" + portOut).c_str());

	mLogger = new LoggingUtility(mOwnerModule, expNo, loggingConf, statisticConf);
}

CommunicationServer::~CommunicationServer() {
	mContext->close();
	mServer->close();
	delete mContext;
	delete mServer;
	delete mLogger;
}


pair<string, string> CommunicationServer::receiveRequest() {
	string envelope = s_recv(*mServer);
	string request = s_recv(*mServer);

	mLogger->logDebug("received request: " + envelope + ", " + request);

	return make_pair(envelope, request);
}

void CommunicationServer::sendReply(string reply) {
	s_send(*mServer, reply);

	mLogger->logDebug("sent reply");
}
