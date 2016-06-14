#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE
#define PRINT_STATS 1

#include "PktStatsCollector.h"
#include <unistd.h>
#include <string>
#include <iostream>
#include <linux/nl80211.h>
#include <net/if.h>

using namespace std;

int PktStatsCollector::mNl80211Id;

PktStatsCollector::PktStatsCollector(string ifname, double probeInterval, boost::asio::io_service* io) {
	mProbeInterval = probeInterval;
	mIfname = ifname;
	mIoService = io;
	mLogger = new LoggingUtility("PktStatsCollector");
	mTimer = new boost::asio::deadline_timer(*mIoService, boost::posix_time::millisec(probeInterval * 1000));
}

PktStatsCollector::~PktStatsCollector() {
	mTimer->cancel();
	delete mTimer;

	free(mWifi);
	nl_socket_modify_cb(mSocket, NL_CB_VALID, NL_CB_CUSTOM,
			NULL, NULL);
	nl_socket_free(mSocket);
	mSocket = NULL;
}

void PktStatsCollector::init() {
	int ret;
	mWifi = (netinterface*) calloc(1, sizeof(netinterface));
	mWifi->ifindex = if_nametoindex(mIfname.c_str());
	if (mWifi->ifindex == 0) {
		mLogger->logError("Error getting interface index for : " + mIfname );
		mWifi->ifindex = -1;
		exit(1);
	}

	// initialize socket now
	mSocket = nl_socket_alloc();
	if (mSocket == NULL) {
		mLogger->logError("Could not allocate netlink socket");
		exit(1);
	}

	// disable sequence number checking. We want to receive notifications.
	nl_socket_disable_seq_check(mSocket);

	ret = nl_socket_modify_cb(mSocket, NL_CB_VALID, NL_CB_CUSTOM,
			PktStatsCollector::receivedNetlinkMsg, this);
	if(ret != 0) {
		mLogger->logError("Failed to modify the callback");
		exit(1);
	}
	// connect socket. Protocol: generic netlink
	ret = genl_connect(mSocket);
	if (ret != 0) {
		mLogger->logError("Connection to netlink socket failed");
		exit(1);
	}

	// resolve mNl80211Id
	mNl80211Id = genl_ctrl_resolve(mSocket, "nl80211");
	if (mNl80211Id < 0) {
		mLogger->logError("Could not get NL80211 id");
		exit(1);
	}
	mLogger->logStats("BE flush required \tBE flush not required \tBK flush required \tBK flush not required \tVI flush required \tVI flush not required \tVO flush required \tVO flush not required");
	mTimer->async_wait(boost::bind(&PktStatsCollector::probe, this, boost::asio::placeholders::error));
}

int PktStatsCollector::receivedNetlinkMsg(nl_msg *msg, void *arg) {
	PktStatsCollector* cp = (PktStatsCollector*) arg;

	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = (genlmsghdr*) nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *sinfo[NL80211_FLUSH_INFO_MAX + 1];
	char dev[20];

    uint32_t be_flush_req, be_flush_not_req;
    uint32_t bk_flush_req, bk_flush_not_req;
    uint32_t vi_flush_req, vi_flush_not_req;
    uint32_t vo_flush_req, vo_flush_not_req;

	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
			genlmsg_attrlen(gnlh, 0), NULL);

	if_indextoname(nla_get_u32(tb[NL80211_ATTR_IFINDEX]), dev);

	if (!tb[NL80211_ATTR_FLUSH_INFO]) {
		cp->mLogger->logInfo("PktStatsCollector: survey data missing!");
		return NL_SKIP;
	}
	static struct nla_policy survey_policy[NL80211_FLUSH_INFO_MAX + 1] = { };

	if (nla_parse_nested(sinfo, NL80211_FLUSH_INFO_MAX,
			tb[NL80211_ATTR_FLUSH_INFO], survey_policy)) {
		cp->mLogger->logInfo("Failed to parse nested attributes");
		return NL_SKIP;
	}

	if (sinfo[NL80211_FLUSH_REQ_BE]) {
		be_flush_req = nla_get_u32(sinfo[NL80211_FLUSH_REQ_BE]);
		if(PRINT_STATS) {
			cout << "\tBE be_flush_req:\t\t\t" << be_flush_req << endl;
		}
	}

	if (sinfo[NL80211_FLUSH_NOT_REQ_BE]) {
		be_flush_not_req = nla_get_u32(sinfo[NL80211_FLUSH_NOT_REQ_BE]);
		if(PRINT_STATS) {
			cout << "\tBE be_flush_not_req:\t\t\t" << be_flush_not_req << endl;
		}
	}

	if (sinfo[NL80211_FLUSH_REQ_BK]) {
		bk_flush_req = nla_get_u32(sinfo[NL80211_FLUSH_REQ_BK]);
		if(PRINT_STATS) {
			cout << "\tBK bk_flush_req:\t\t\t" << bk_flush_req << endl;
		}
	}
	if (sinfo[NL80211_FLUSH_NOT_REQ_BK]) {
		bk_flush_not_req = nla_get_u32(sinfo[NL80211_FLUSH_NOT_REQ_BK]);
		if(PRINT_STATS) {
			cout << "\tBK bk_flush_not_req:\t\t\t" << bk_flush_not_req << endl;
		}
	}

	if (sinfo[NL80211_FLUSH_REQ_VI]) {
		vi_flush_req = nla_get_u32(sinfo[NL80211_FLUSH_REQ_VI]);
		if(PRINT_STATS) {
			cout << "\tVI vi_flush_req:\t\t\t" << vi_flush_req << endl;
		}
	}
	if (sinfo[NL80211_FLUSH_NOT_REQ_VI]) {
		vi_flush_not_req = nla_get_u32(sinfo[NL80211_FLUSH_NOT_REQ_VI]);
		if(PRINT_STATS) {
			cout << "\tVI vi_flush_not_req:\t\t\t" << vi_flush_not_req << endl;
		}
	}

	if (sinfo[NL80211_FLUSH_REQ_VO]) {
		vo_flush_req = nla_get_u32(sinfo[NL80211_FLUSH_REQ_VO]);
		if(PRINT_STATS) {
			cout << "\tVO vo_flush_req:\t\t\t" << vo_flush_req << endl;
		}
	}
	if (sinfo[NL80211_FLUSH_NOT_REQ_VO]) {
		vo_flush_not_req = nla_get_u32(sinfo[NL80211_FLUSH_NOT_REQ_VO]);
		if(PRINT_STATS) {
			cout << "\tVO vo_flush_not_req:\t\t\t" << vo_flush_not_req << endl;
		}
	}

	// Update statistics
//	uint64_t busy_time_diff = busy_time - cp->mWifi->stats.busyTimeLast;
//	uint64_t total_time_diff = total_time - cp->mWifi->stats.totalTimeLast;
//	double load = ((100 * busy_time_diff) / total_time_diff) / 100.0;
//	cp->mWifi->stats.mutexChannelLoad.lock();
//	cp->mWifi->stats.load = load;
//	cp->mWifi->stats.totalTimeLast = total_time;
//	cp->mWifi->stats.busyTimeLast = busy_time;
//	cp->mWifi->stats.noise = noise;
//	cp->mWifi->stats.mutexChannelLoad.unlock();
//	cp->mLogger->logStats(to_string(channel) + "\t" + to_string(busy_time_diff) + "\t" + to_string(total_time_diff) + "\t" + to_string(load));
	return NL_SKIP;
}

void PktStatsCollector::probe(const boost::system::error_code &ec) {
	sendNl80211(NL80211_CMD_FLUSH_STATS, &mWifi->ifindex,
			sizeof(mWifi->ifindex), NL80211_ATTR_IFINDEX, NL_AUTO_SEQ,
			NLM_F_DUMP);
	nl_recvmsgs_default(mSocket);

	mTimer->expires_from_now(boost::posix_time::millisec(mProbeInterval * 1000));
	mTimer->async_wait(boost::bind(&PktStatsCollector::probe, this, boost::asio::placeholders::error));
}

int PktStatsCollector::sendNl80211(uint8_t msgCmd, void *payload,
		unsigned int length, int payloadType, unsigned int seq, int flags) {
	return send(msgCmd, payload, length, payloadType, seq, mNl80211Id, flags,
			0x01);
}

int PktStatsCollector::send(uint8_t msgCmd, void *payload, unsigned int length,
		int attrType, unsigned int seq, int protocolId, int flags,
		uint8_t protocolVersion) {
	int ret = 0;

	// create a new Netlink message
	nl_msg *msg = nlmsg_alloc();

	// create message header
	if (genlmsg_put(msg, 0, seq, protocolId, 0, flags, msgCmd,
			protocolVersion) == NULL) {
		mLogger->logError("Failed to create netlink header for the message");
		return -1;
	}

	// add message attributes (=payload)
	if (length > 0) {
		ret = nla_put(msg, attrType, length, payload);
		if (ret < 0) {
			mLogger->logError("Error when adding attributes");
			return ret;
		}
	}

	ret = nl_send_auto(mSocket, msg);
	if (ret < 0) {
		mLogger->logError("Error when sending the message");
	}
	nlmsg_free(msg);

	return ret;
}

double PktStatsCollector::getChannelLoad() {
	float load;
	mWifi->stats.mutexChannelLoad.lock();
	// TODO
	mWifi->stats.mutexChannelLoad.unlock();
	return (double) load;
}
