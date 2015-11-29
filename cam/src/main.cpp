#include "cam.h"
#include <csignal>
#include <iostream>

using namespace std;

void onSigTermOk(int signum) {
	cout << "Signal " << signum << " received. Requesting exit." << std::endl;
	exit(EXIT_SUCCESS);
}

void onSigMsg(int signum) {
	cout << "Signal " << signum << " received." << std::endl;
}

void onSigTermCrash(int signum) {
	cout << "Signal " << signum << " received. Crashed!" << std::endl;
	exit(EXIT_FAILURE);
}

void installSignals() {
	signal(SIGHUP, &onSigMsg);
	signal(SIGINT, &onSigTermOk);
	signal(SIGQUIT, &onSigTermOk);
	signal(SIGILL, &onSigMsg);
	signal(SIGABRT, &onSigTermOk);
	signal(SIGFPE, &onSigMsg);
	signal(SIGKILL, &onSigTermOk);
	signal(SIGSEGV, &onSigTermCrash);
	signal(SIGPIPE, &onSigMsg);
	signal(SIGALRM, &onSigMsg);
	signal(SIGTERM, &onSigTermOk);
	signal(SIGUSR1, &onSigMsg);
	signal(SIGUSR2, &onSigMsg);
	signal(SIGCHLD, &onSigMsg);
	signal(SIGCONT, &onSigMsg);
	signal(SIGSTOP, &onSigMsg);
	signal(SIGTSTP, &onSigMsg);
	signal(SIGTTIN, &onSigMsg);
	signal(SIGTTOU, &onSigMsg);
	cout << "Signals installed" << endl;
}

int main(int argc, char* argv[]) {
	installSignals();

	boost::shared_ptr<CAM> camPtr = CAM::createCAM();
	camPtr->init();
	camPtr->start();

	return EXIT_SUCCESS;
}
