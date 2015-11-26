#include <signal.h>
#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <zmq.hpp>

#include "cam.h"

using namespace std;

void
on_sig_term_ok (int piSignum)
{
  cout << "Signal " << piSignum << " received. Requesting exit." << std::endl;
  exit (EXIT_SUCCESS);
}

void
on_sig_msg (int piSignum)
{
  cout << "Signal " << piSignum << " received." << std::endl;
}

void
on_sig_term_crash (int piSignum)
{
  cout << "Signal " << piSignum << " received. Crashed!" << std::endl;
  exit (-1);
}

void
installSignals ()
{
  signal (SIGHUP, &on_sig_msg);
  signal (SIGINT, &on_sig_term_ok);
  signal (SIGQUIT, &on_sig_term_ok);
  signal (SIGILL, &on_sig_msg);
  signal (SIGABRT, &on_sig_term_ok);
  signal (SIGFPE, &on_sig_msg);
  signal (SIGKILL, &on_sig_term_ok);
  signal (SIGSEGV, &on_sig_term_crash);
  signal (SIGPIPE, &on_sig_msg);
  signal (SIGALRM, &on_sig_msg);
  signal (SIGTERM, &on_sig_term_ok);
  signal (SIGUSR1, &on_sig_msg);
  signal (SIGUSR2, &on_sig_msg);
  signal (SIGCHLD, &on_sig_msg);
  signal (SIGCONT, &on_sig_msg);
  signal (SIGSTOP, &on_sig_msg);
  signal (SIGTSTP, &on_sig_msg);
  signal (SIGTTIN, &on_sig_msg);
  signal (SIGTTOU, &on_sig_msg);
}

int
main ()
{
  installSignals();

  CAM cam;
  cam.loop();

  return EXIT_SUCCESS;
}
