#ifndef CHANNELPROBER_H_
#define CHANNELPROBER_H_

/**
 * @addtogroup dccHardware
 * @{
 */

#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <utility/LoggingUtility.h>
#include <netlink/netlink-kernel.h>
#include <netlink/socket.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/mngt.h>

/**
 * ChannelProber probes the Ath9k NIC for the current channel load periodically via netlink.
 */
class ChannelProber {
public:
	ChannelProber(std::string ifname, double probeInterval, boost::asio::io_service* io, int expNo);
	virtual ~ChannelProber();

	void init();
	void probe(const boost::system::error_code &ec);
	int sendNl80211(uint8_t msgCmd, void *payload, unsigned int length, int payloadType, unsigned int seq, int flags = 0);
	int send(uint8_t msgCmd, void *payload, unsigned int length, int attrType, unsigned int seq, int protocolId, int flags = 0, uint8_t protocolVersion = 0x01);
	double getChannelLoad();
	static int receivedNetlinkMsg(nl_msg *msg, void *arg);

	/**
	 * Struct that holds the latest information for channel load.
	 */
	struct channelload {
		boost::mutex mutexChannelLoad;
		uint8_t noise;
		uint64_t totalTimeLast;
		uint64_t busyTimeLast;
		double load;
	};
	/**
	 * Struct for latest channel load specific to an interface.
	 */
	struct netinterface {
		unsigned int ifindex;
		unsigned int channel;
		channelload load;
	};

	nl_sock *mSocket;
	static int mNl80211Id;

	netinterface *mWifi;
	std::string mIfname;
	double mProbeInterval;
	LoggingUtility* mLogger;

private:
	boost::asio::io_service* mIoService;
	boost::asio::deadline_timer* mTimer;
};

/**
 * @}
 */

#endif
