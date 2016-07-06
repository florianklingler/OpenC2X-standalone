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
struct netinterface {
	unsigned int ifindex;
	PktStats stats;
};

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
