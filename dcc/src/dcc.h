#include "zmq.hpp"
#include "zhelpers.hpp"

using namespace std;
using namespace zmq;
class DCC
{
public:
  DCC ();

  ~DCC ();

  void
  loop ();

private:
  context_t* context;
  socket_t* publisher;

};
