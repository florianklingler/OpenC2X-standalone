#ifndef CHANNELPROBER_H_
#define CHANNELPROBER_H_

#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <utility/LoggingUtility.h>
#include <netlink/netlink-kernel.h>
#include <netlink/socket.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/mngt.h>

class ChannelProber {
public:
	ChannelProber(string ifname, double probeInterval, boost::asio::io_service* io);
	virtual ~ChannelProber();

	void init();
	void probe(const boost::system::error_code &ec);
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

	netinterface *mWifi;
	string mIfname;
	double mProbeInterval;
	LoggingUtility* mLogger;

private:
	boost::asio::io_service* mIoService;
	boost::asio::deadline_timer* mTimer;
};

#endif
