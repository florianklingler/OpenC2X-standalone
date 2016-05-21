#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include "denmApp.h"
#include <string>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

using namespace std;

INITIALIZE_EASYLOGGINGPP

DenmApp::DenmApp() {
	mLogger = new LoggingUtility("DenmApp");
	mSenderToDenm = new CommunicationSender("DenmApp", "1111");
}

DenmApp::~DenmApp() {
	delete mLogger;
	delete mSenderToDenm;
}

void DenmApp::triggerDenm(string content) {
	triggerPackage::TRIGGER trigger;
	string serializedTrigger;

	trigger.set_content(content);

	trigger.SerializeToString(&serializedTrigger);
	mSenderToDenm->send("TRIGGER", serializedTrigger);
}



int main(int argc, char ** argv) {
	DenmApp denmApp;

	cout << "Welcome to denmApp!" << endl;

	while(1) {
		cout << "Enter content of DENM:" << endl;
		char * charLine = readline("> ");		//read line
		if(!charLine) break;
		if(*charLine) add_history(charLine);
		string line(charLine);					//convert to string for further use
		free(charLine);
		if (line.compare("exit") == 0) return EXIT_SUCCESS;	//TODO: fix

		denmApp.triggerDenm(line);
	}

	return EXIT_SUCCESS;
}
