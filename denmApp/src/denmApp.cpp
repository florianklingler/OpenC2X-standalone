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
}

DenmApp::~DenmApp() {
	delete mLogger;
}

void DenmApp::init() {

}



int main(int argc, char ** argv) {
//	DenmApp denmApp;
//	denmApp.init();

	cout << "Welcome to denmApp!" << endl;

	while(true) {
		char * charLine = readline("> ");		//read line
		if(!charLine) break;
		if(*charLine) add_history(charLine);
		string line(charLine);					//convert to string for further use
		free(charLine);

		if (line.compare("exit") == 0) break;
	}

	return EXIT_SUCCESS;
}
