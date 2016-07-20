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


#include "CommunicationReceiver.h"

using namespace std;

CommunicationReceiver::CommunicationReceiver(string ownerModule, string portIn, string envelope, int expNo) {
	mOwnerModule = ownerModule;

	mEnvelope = envelope;
	mContext = new zmq::context_t(1);
	mSubscriber = new zmq::socket_t(*mContext, ZMQ_SUB);
	mSubscriber->connect(("tcp://localhost:" + portIn).c_str());
	if (envelope == "") {
		mSubscriber->setsockopt(ZMQ_SUBSCRIBE, "", 0);//subscribe to all messages
	} else {
		mSubscriber->setsockopt(ZMQ_SUBSCRIBE, envelope.c_str(), 1);
	}

	mLogger = new LoggingUtility(mOwnerModule, expNo);
}

CommunicationReceiver::~CommunicationReceiver() {
	mContext->close();
	mSubscriber->close();
	delete mContext;
	delete mSubscriber;
	delete mLogger;
}

pair<string, string> CommunicationReceiver::receive() {
	string envelope = s_recv(*mSubscriber);
	string message = s_recv(*mSubscriber);

	mLogger->logDebug(envelope + " received");

	return make_pair(envelope, message);
}

string CommunicationReceiver::receiveFromHw() {
	string message = s_recv(*mSubscriber);

	mLogger->logDebug("received from HW");

	return message;
}

string CommunicationReceiver::receiveData() {
	string envelope = s_recv(*mSubscriber);
	string message = s_recv(*mSubscriber);
	return message;
}

