#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include "ChannelProber.h"
#include <unistd.h>
#include <string>
#include <iostream>
#include <linux/nl80211.h>
#include <net/if.h>

using namespace std;

int ChannelProber::mNl80211Id;

ChannelProber::ChannelProber(string ifname, double probeInterval, boost::asio::io_service* io, int expNo) {
	mProbeInterval = probeInterval;
	mIfname = ifname;
	mIoService = io;
	mLogger = new LoggingUtility("ChannelProber", expNo);
	mTimer = new boost::asio::deadline_timer(*mIoService, boost::posix_time::millisec(probeInterval * 1000));
}

ChannelProber::~ChannelProber() {
	mTimer->cancel();
	delete mTimer;

	free(mWifi);
	nl_socket_modify_cb(mSocket, NL_CB_VALID, NL_CB_CUSTOM,
			NULL, NULL);
	nl_socket_free(mSocket);
	mSocket = NULL;
}

void ChannelProber::init() {
	int ret;
	mWifi = (netinterface*) calloc(1, sizeof(netinterface));
	mWifi->ifindex = if_nametoindex(mIfname.c_str());
	if (mWifi->ifindex == 0) {
		mLogger->logError("Error getting interface index for : " + mIfname );
		mWifi->ifindex = -1;
		mWifi->channel = -1;
		exit(1);
	}
	mWifi->load.load = -1;
	mWifi->load.totalTimeLast = 0;
	mWifi->load.busyTimeLast = 0;

	// initialize socket now
	mSocket = nl_socket_alloc();
	if (mSocket == NULL) {
		mLogger->logError("Could not allocate netlink socket");
		exit(1);
	}

	// disable sequence number checking. We want to receive notifications.
	nl_socket_disable_seq_check(mSocket);

	ret = nl_socket_modify_cb(mSocket, NL_CB_VALID, NL_CB_CUSTOM,
			ChannelProber::receivedNetlinkMsg, this);
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
	mLogger->logStats("Channel \tBusy time \tTotal time \tChannel load");
	mTimer->async_wait(boost::bind(&ChannelProber::probe, this, boost::asio::placeholders::error));
}

int ChannelProber::receivedNetlinkMsg(nl_msg *msg, void *arg) {
	ChannelProber* cp = (ChannelProber*) arg;

	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = (genlmsghdr*) nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *sinfo[NL80211_SURVEY_INFO_MAX + 1];
	char dev[20];

	uint64_t total_time = 0, busy_time = 0;
	uint32_t channel = 0;
	int8_t noise;

	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
			genlmsg_attrlen(gnlh, 0), NULL);

	if_indextoname(nla_get_u32(tb[NL80211_ATTR_IFINDEX]), dev);

	if (!tb[NL80211_ATTR_SURVEY_INFO]) {
		cp->mLogger->logInfo("ChannelProber: survey data missing!");
		return NL_SKIP;
	}
	static struct nla_policy survey_policy[NL80211_SURVEY_INFO_MAX + 1] = { };

	if (nla_parse_nested(sinfo, NL80211_SURVEY_INFO_MAX,
			tb[NL80211_ATTR_SURVEY_INFO], survey_policy)) {
		cp->mLogger->logInfo("Failed to parse nested attributes");
		return NL_SKIP;
	}

	// If this info is not about the channel in use, then skip
	if (!sinfo[NL80211_SURVEY_INFO_IN_USE]) {
		return NL_SKIP;
	}

	// Current channel in use
	if (sinfo[NL80211_SURVEY_INFO_FREQUENCY]) {
		channel = nla_get_u32(sinfo[NL80211_SURVEY_INFO_FREQUENCY]);
	}

	// Noise
	if (sinfo[NL80211_SURVEY_INFO_NOISE]) {
		noise = (int8_t) nla_get_u8(sinfo[NL80211_SURVEY_INFO_NOISE]);
	}

	// Total active time
	if (sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME]) {
		total_time = nla_get_u64(sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME]);
	}

	// Total busy time
	if (sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME_BUSY]) {
		busy_time = nla_get_u64(sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME_BUSY]);
	}

	// Do we need info about extension channel?
	if (sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME_EXT_BUSY]) {
		// cout << "\textension channel busy time:\t"
		//		<< nla_get_u64(sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME_EXT_BUSY])
		//		<< " ms" << endl;
	}

	// Total receiving time
	if (sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME_RX]) {
		//cout << "\tchannel receive time:\t\t"
		//	<< nla_get_u64(sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME_RX])
		//	<< " ms" << endl;
	}

	// Total transmitting time
	if (sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME_TX]) {
		//cout << "\tchannel transmit time:\t\t"
		//	<< nla_get_u64(sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME_TX])
		//	<< " ms" << endl;
	}

	// Update statistics
	uint64_t busy_time_diff = busy_time - cp->mWifi->load.busyTimeLast;
	uint64_t total_time_diff = total_time - cp->mWifi->load.totalTimeLast;
	double load = ((100 * busy_time_diff) / total_time_diff) / 100.0;
	cp->mWifi->load.mutexChannelLoad.lock();
	cp->mWifi->load.load = load;
	cp->mWifi->load.totalTimeLast = total_time;
	cp->mWifi->load.busyTimeLast = busy_time;
	cp->mWifi->load.noise = noise;
	cp->mWifi->load.mutexChannelLoad.unlock();
//	cp->mLogger->logStats(to_string(channel) + "\t" + to_string(busy_time_diff) + "\t" + to_string(total_time_diff) + "\t" + to_string(load));
	return NL_SKIP;
}

void ChannelProber::probe(const boost::system::error_code &ec) {
	sendNl80211(NL80211_CMD_GET_SURVEY, &mWifi->ifindex,
			sizeof(mWifi->ifindex), NL80211_ATTR_IFINDEX, NL_AUTO_SEQ,
			NLM_F_DUMP);
	nl_recvmsgs_default(mSocket);

	mTimer->expires_from_now(boost::posix_time::millisec(mProbeInterval * 1000));
	mTimer->async_wait(boost::bind(&ChannelProber::probe, this, boost::asio::placeholders::error));
}

int ChannelProber::sendNl80211(uint8_t msgCmd, void *payload,
		unsigned int length, int payloadType, unsigned int seq, int flags) {
	return send(msgCmd, payload, length, payloadType, seq, mNl80211Id, flags,
			0x01);
}

int ChannelProber::send(uint8_t msgCmd, void *payload, unsigned int length,
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

double ChannelProber::getChannelLoad() {
	float load;
	mWifi->load.mutexChannelLoad.lock();
	load = mWifi->load.load;
	mWifi->load.mutexChannelLoad.unlock();
	return (double) load;
}
