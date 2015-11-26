#include "cam.h"
#include "zhelpers.hpp"
#include <iostream>
#include <unistd.h>

using namespace std;

void
CAM::loop ()
{
  //  Prepare our context and subscriber
  zmq::context_t context (1);
  zmq::socket_t subscriber (context, ZMQ_SUB);
  subscriber.connect ("tcp://localhost:5563");
  subscriber.setsockopt ( ZMQ_SUBSCRIBE, "B", 1);

  while (1)
    {
      //  Read envelope with address
      std::string address = s_recv (subscriber);
      //  Read message contents
      std::string contents = s_recv (subscriber);
      std::cout << "[" << address << "] " << contents << std::endl;
    }
}
