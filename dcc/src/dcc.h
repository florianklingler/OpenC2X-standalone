#include <zmq.hpp>
class DCC
{
public:
  DCC ();

  ~DCC ();

  void
  loop ();

private:
  zmq::context_t* context;
  zmq::socket_t* publisher;

};
