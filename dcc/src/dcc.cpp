#include "dcc.h"

#include <iostream>
#include <unistd.h>
#include <zmq.hpp>

DCC::DCC ()
{
//  context = new context_t(1);
//  publisher = new socket_t(*context, ZMQ_PUB);
//  publisher->bind("tcp://*.5563");
}

DCC::~DCC ()
{
//  publisher->unbind(*context);
//  delete *context;
}

void
DCC::loop ()
{
  zmq::context_t context(1);
  zmq::socket_t publisher(context, ZMQ_PUB);
  publisher.bind("tcp://*:5563");
  while (1)
    {
      cout << "sending signal A!\n";
      s_sendmore (publisher, "A");
      s_send (publisher, "We don't want to see this");
      cout << "sending signal B!\n";
      s_sendmore (publisher, "B");
      s_send (publisher, "We would like to see this");
      cout << endl;
      sleep (1);
    }
}
