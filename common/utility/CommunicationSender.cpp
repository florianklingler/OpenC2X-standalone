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


#include "CommunicationSender.h"

using namespace std;

CommunicationSender::CommunicationSender(string ownerModule, string portOut, int expNo) {
	mOwnerModule = ownerModule;

	mContext = new zmq::context_t(1);
	mPublisher = new zmq::socket_t(*mContext, ZMQ_PUB);
	mPublisher->bind(("tcp://*:" + portOut).c_str());

	mLogger = new LoggingUtility(mOwnerModule, expNo);
}

CommunicationSender::~CommunicationSender() {
	mContext->close();
	mPublisher->close();
	delete mContext;
	delete mPublisher;
	delete mLogger;
}

void CommunicationSender::send(string envelope, string message) {
	s_sendmore(*mPublisher, envelope);
	s_send(*mPublisher, message);

	mLogger->logDebug(envelope + " sent");
}

void CommunicationSender::sendToHw(string message) {
	s_send(*mPublisher, message);

	mLogger->logDebug("sent to HW");
}

void CommunicationSender::sendData(string envelope, string message) {
	s_sendmore(*mPublisher, envelope);
	s_send(*mPublisher, message);
}
