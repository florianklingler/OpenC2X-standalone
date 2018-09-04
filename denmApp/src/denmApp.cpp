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


#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include "denmApp.h"
#include <common/config/config.h>

using namespace std;

INITIALIZE_EASYLOGGINGPP

DenmApp::DenmApp() {
	GlobalConfig config;
	try {
		config.loadConfig(DENM_CONFIG_NAME);
	}
	catch (std::exception &e) {
		cerr << "Error while loading /etc/config/openc2x_common: " << e.what() << endl;
	}
	ptree pt = load_config_tree();
	mLogger = new LoggingUtility(DENM_CONFIG_NAME, DENM_APP_MODULE_NAME, config.mLogBasePath, config.mExpName, config.mExpNo, pt);

	mSenderToDenm = new CommunicationSender("1111", *mLogger);
}

DenmApp::~DenmApp() {
	delete mSenderToDenm;
}

void DenmApp::triggerDenm(string content) {
	triggerPackage::TRIGGER trigger;
	string serializedTrigger;

	trigger.set_content(content);

	trigger.SerializeToString(&serializedTrigger);
	mSenderToDenm->send("TRIGGER", serializedTrigger);
}



int main(int argc, const char* argv[]) {

	DenmApp denmApp;

	usleep(200*1000);	//FIXME: zmq seems to need some time for setup => doesn't send without sleep

	string content = "test denm app";
	denmApp.triggerDenm(content);

	exit(0);			//FIXME: CommunicationSender doesn't terminate => program doesn't quit without exit

	return 0;
}
