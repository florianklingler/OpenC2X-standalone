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


#ifndef DENMAPP_H_
#define DENMAPP_H_

#include <string>
#include <stdio.h>
#include <unistd.h>
#include <common/utility/CommunicationSender.h>
#include <common/utility/Constants.h>
#include <common/buffers/trigger.pb.h>

/*
 * This is a simple application that can trigger DENMs using a command-line interface.
 * It is not used in the current stack. If you want to use it, compile and
 * run "./denmApp <content>" from within denmApp/Debug.
 */
class DenmApp {
public:
	DenmApp();
	~DenmApp();

	/*
	 * Sends a request to trigger a DENM with the specified content to the DenService.
	 * @param content The content of the DENM to be triggered.
	 */
	void triggerDenm(std::string content);

private:
	CommunicationSender* mSenderToDenm;
	LoggingUtility* mLogger;
};

#endif
