#include "dcc.h"
#include <execinfo.h>
#include <csignal>
#include <iostream>

#define MAX_BACKTRACE_ENTRIES 50

using namespace std;

void printBacktrace() {
  void* array[MAX_BACKTRACE_ENTRIES];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, MAX_BACKTRACE_ENTRIES);

  // print out all the frames to stderr
  std::cerr << "Backtrace:" << std::endl;
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  std::cerr << std::endl;
  fflush(NULL);
}

void onSigTermOk(int piSignum) {
	cout << "Signal " << piSignum << " received. Requesting exit." << std::endl;
	printBacktrace();
	exit(EXIT_SUCCESS);
}

void onSigMsg(int piSignum) {
	cout << "Signal " << piSignum << " received." << std::endl;
}

void onSigTermCrash(int piSignum) {
	cout << "Signal " << piSignum << " received. Crashed!" << std::endl;
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
}

int main() {
	installSignals();

	boost::shared_ptr<DCC> dccPtr = DCC::createDCC();
	dccPtr->init();
	dccPtr->start();

	return EXIT_SUCCESS;
}
