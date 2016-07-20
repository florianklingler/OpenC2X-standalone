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
	/**
	 * Number of times an older packet was found in the hardware AC_BE before adding a new one
	 */
	uint32_t be_flush_req;
	/**
	 * Number of times an older packet was not found in the hardware AC_BE before adding a new one
	 */
	uint32_t be_flush_not_req;

	/**
	 * Number of times an older packet was found in the hardware AC_BK before adding a new one
	 */
	uint32_t bk_flush_req;

	/**
	 * Number of times an older packet was not found in the hardware AC_BK before adding a new one
	 */
	uint32_t bk_flush_not_req;

	/**
	 * Number of times an older packet was found in the hardware AC_VI before adding a new one
	 */
	uint32_t vi_flush_req;

	/**
	 * Number of times an older packet was not found in the hardware AC_VI before adding a new one
	 */
	uint32_t vi_flush_not_req;

	/**
	 * Number of times an older packet was found in the hardware AC_VO before adding a new one
	 */
	uint32_t vo_flush_req;

	/**
	 * Number of times an older packet was not found in the hardware AC_VO before adding a new one
	 */
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
 * in the hardware queues in NIC. The Ath9k chip does not support on demand flushing of hardware queues. But there
 * might be scenarios (e.g. for CAMs) where outdated packets, which could not make their way due to heavy channel load
 * need to be flushed.
 * This implementation is specific to Ath9k chipset, with modified Linux Kernel 3.18.
 *
 * @todo When spamming the channel to 100% utilization, the kernel debug stats (e.g. TX-Pkts-All, HW-tx-start,
 * HW-put-tx-buf etc.) for transmission keep on increasing and TX-Failed stays 0. Because of insufficient documentation,
 * the reason behind such behavior is still unknown. So, at present, PktStatCollector results could not be verified. The
 * ath9k behavior under heavy channel utilizations need to be analyzed.
 */
class PktStatsCollector {
public:
	/**
	 * Constructor.
	 * @param ifname WLan interface name
	 * @param probeInterval time interval between two consecutive probes
	 * @param io boost io service
	 * @param expNo experiment number
	 */
	PktStatsCollector(std::string ifname, double probeInterval, boost::asio::io_service* io, int expNo);
	virtual ~PktStatsCollector();

	/**
	 * Initializes PktStatsCollector.
	 * Allocate a netlink socket, connect to it and start probing.
	 */
	void init();

	/**
	 * Starts collecting the statistics.
	 * @param ec Boost error code
	 */
	void probe(const boost::system::error_code &ec);

	/**
	 * Send a netlink message via nl80211 module.
	 * @param msgCmd The nl80211 command
	 * @param payload Netlink data
	 * @param length length of the payload
	 * @param payloadType The nl80211 attribute
	 * @param seq The sequence number for netlink
	 * @param flags Netlink flags
	 * @return 0 if success, < 0 otherwise.
	 */
	int sendNl80211(uint8_t msgCmd, void *payload, unsigned int length, int payloadType, unsigned int seq, int flags = 0);

	/**
	 * Creates a new netlink message, send it via nl80211 module.
	 * @param msgCmd The nl80211 command
	 * @param payload Netlink data
	 * @param length Length of the payload
	 * @param attrType The nl80211 attribute
	 * @param seq The sequence number for netlink
	 * @param protocolId The protocol id
	 * @param flags Netlink flags
	 * @param protocolVersion protocolversion
	 * @return 0 if success, < 0 otherwise.
	 */
	int send(uint8_t msgCmd, void *payload, unsigned int length, int attrType, unsigned int seq, int protocolId, int flags = 0, uint8_t protocolVersion = 0x01);

	/** Receives the netlink message from the kernel.
	 * The received message is parsed and the current information about the channel utilization is saved in the channelload struct.
	 * @param msg Received netlink message corresponding to the probe
	 * @param arg Pointer to PktStatsCollector
	 * @return 1
	 */
	static int receivedNetlinkMsg(nl_msg *msg, void *arg);

	/**
	 * Get the statistics.
	 * @return The measured statistics.
	 */
	PktStats getPktStats();

	/**
	 * Netlink socket
	 */
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
