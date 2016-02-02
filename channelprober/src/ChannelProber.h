#ifndef CHANNELPROBER_H_
#define CHANNELPROBER_H_

#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
#include <utility/LoggingUtility.h>
#include <netlink/netlink-kernel.h>
#include <netlink/socket.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/mngt.h>

class ChannelProber {
public:
	ChannelProber();
	virtual ~ChannelProber();

	void init();
	void probe();
	int send_nl80211(uint8_t msgCmd, void *payload, unsigned int pLength, int payloadType, unsigned int seq, int flags = 0);
	int send(uint8_t msgCmd, void *payload, unsigned int pLength, int attrType, unsigned int seq, int protocol_id, int flags = 0, uint8_t protocol_version = 0x01);

	// Callback functions for netlink. Not to be used by user!
	static int receivedNetlinkMsg(nl_msg *msg, void *arg);

	struct channelload {
		boost::mutex mutex_channelLoad;
		uint8_t noise;
		uint64_t total_time_last;
		uint64_t busy_time_last;
		double load;
	};
	struct net_interface {
		unsigned int ifindex;
		unsigned int channel;
		channelload load;
	};

	nl_sock *mSocket;
	int socket_state;
	static int mNl80211Id;
	boost::thread* mThreadProbe;

	net_interface *wifi;
	int mTotalWifi;
	unsigned int mChannelLoadProbingPeriod;
};

#endif
