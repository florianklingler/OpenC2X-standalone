#ifndef PKT_STATS_COLLECTOR_H_
#define PKT_STATS_COLLECTOR_H_

#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
#include <netlink/netlink-kernel.h>
#include <netlink/socket.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/mngt.h>

using namespace std;

class PktStatsCollector {
public:
	PktStatsCollector(string ifname);
	virtual ~PktStatsCollector();

	void init();
	void probe();
	int sendNl80211(uint8_t msgCmd, void *payload, unsigned int length, int payloadType, unsigned int seq, int flags = 0);
	int send(uint8_t msgCmd, void *payload, unsigned int length, int attrType, unsigned int seq, int protocolId, int flags = 0, uint8_t protocolVersion = 0x01);
	double getChannelLoad();
	static int receivedNetlinkMsg(nl_msg *msg, void *arg);

	struct channelload {
		boost::mutex mutexChannelLoad;
		uint8_t noise;
		uint64_t totalTimeLast;
		uint64_t busyTimeLast;
		double load;
	};
	struct netinterface {
		unsigned int ifindex;
		unsigned int channel;
		channelload load;
	};

	nl_sock *mSocket;
	static int mNl80211Id;
	boost::thread* mThreadProbe;

	netinterface *mWifi;
	string mIfname;
};

#endif
