#ifndef PKT_STATS_COLLECTOR_H_
#define PKT_STATS_COLLECTOR_H_

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
 * Struct that holds the latest information about the flush statistics for all hw queues.
 */
struct PktStats {
	uint32_t be_flush_req;
	uint32_t be_flush_not_req;
	uint32_t bk_flush_req;
	uint32_t bk_flush_not_req;
	uint32_t vi_flush_req;
	uint32_t vi_flush_not_req;
	uint32_t vo_flush_req;
	uint32_t vo_flush_not_req;
};

/**
 * Netlink interface for collecting packet stats.
 */
struct netinterface {
	unsigned int ifindex;
	PktStats stats;
};

/**
 * PktStatsCollector collects the statistics for the number of times when there was need to flush an outdated packet
 * in the hardware queues in NIC. This is specific to Ath9k chipset, with modified Linux Kernel 3.18.
 */
class PktStatsCollector {
public:
	PktStatsCollector(std::string ifname, double probeInterval, boost::asio::io_service* io, int expNo);
	virtual ~PktStatsCollector();

	void init();
	void probe(const boost::system::error_code &ec);
	int sendNl80211(uint8_t msgCmd, void *payload, unsigned int length, int payloadType, unsigned int seq, int flags = 0);
	int send(uint8_t msgCmd, void *payload, unsigned int length, int attrType, unsigned int seq, int protocolId, int flags = 0, uint8_t protocolVersion = 0x01);
	static int receivedNetlinkMsg(nl_msg *msg, void *arg);
	PktStats getPktStats();

	nl_sock *mSocket;
	static int mNl80211Id;

	netinterface *mWifi;
	std::string mIfname;
	double mProbeInterval;
	LoggingUtility* mLogger;

private:
	static boost::mutex mutexStats;
	boost::asio::io_service* mIoService;
	boost::asio::deadline_timer* mTimer;
};

/**
 * @}
 */

#endif
