#include "PktStatsCollector.h"
#include <unistd.h>
#include <string>
#include <iostream>
#include <linux/nl80211.h>
#include <net/if.h>

#define PRINT_STATS 1

using namespace std;

int PktStatsCollector::mNl80211Id;

PktStatsCollector::PktStatsCollector(string ifname) {
	mIfname = ifname;
}

PktStatsCollector::~PktStatsCollector() {
	mThreadProbe->join();
	delete mThreadProbe;
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
		cout << "Prober: error getting network interface. errno: " << errno << endl;
		mWifi->ifindex = -1;
		mWifi->channel = -1;
	}
	mWifi->load.load = -1;
	mWifi->load.totalTimeLast = 0;
	mWifi->load.busyTimeLast = 0;

	// initialize socket now
	mSocket = nl_socket_alloc();
	if (mSocket == NULL) {
		cout << "Prober: nl_socket_alloc error" << endl;
		return;
	}

	// disable sequence number checking. We want to receive notifications.
	nl_socket_disable_seq_check(mSocket); // GSP

	ret = nl_socket_modify_cb(mSocket, NL_CB_VALID, NL_CB_CUSTOM,
			PktStatsCollector::receivedNetlinkMsg, this);
	if(ret != 0) {
		cout << "Prober: error when setting callback : " << ret << endl;
	}
	// connect socket. Protocol: generic netlink
	ret = genl_connect(mSocket);
	if (ret != 0) {
		cout << "Prober: Connection to Generic Netlink Socket failed! err: "
				<< nl_geterror(ret) << endl;
		return;
	}

	// resolve mNl80211Id
	mNl80211Id = genl_ctrl_resolve(mSocket, "nl80211");
	if (mNl80211Id < 0) {
		cout << "Prober: error code getting nl80211 id: " << nl_geterror(mNl80211Id)
				<< endl;
		return;
	}

	mThreadProbe = new boost::thread(&PktStatsCollector::probe, this);
}

int PktStatsCollector::receivedNetlinkMsg(nl_msg *msg, void *arg) {
	cout << "Received Netlink Message " << endl;
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
		cout << "Prober: survey data missing!" << endl;
		return NL_SKIP;
	}
	static struct nla_policy survey_policy[NL80211_FLUSH_INFO_MAX + 1] = { };

	if (nla_parse_nested(sinfo, NL80211_FLUSH_INFO_MAX,
			tb[NL80211_ATTR_FLUSH_INFO], survey_policy)) {
		cout << "Prober: failed to parse nested attributes!" << endl;
		return NL_SKIP;
	}
	////////////////////////////////////////////////////////////////////////////////////
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
	////////////////////////////////////////////////////////////////////////////////////

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
	////////////////////////////////////////////////////////////////////////////////////

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
	////////////////////////////////////////////////////////////////////////////////////


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
	////////////////////////////////////////////////////////////////////////////////////
	// Update statistics
	/*
	uint64_t busy_time_diff = busy_time - cp->mWifi->load.busyTimeLast;
	uint64_t total_time_diff = total_time - cp->mWifi->load.totalTimeLast;
	double load = ((100 * busy_time_diff) / total_time_diff) / 100.0;
	cp->mWifi->load.mutexChannelLoad.lock();
	cp->mWifi->load.load = load;
	cp->mWifi->load.totalTimeLast = total_time;
	cp->mWifi->load.busyTimeLast = busy_time;
	cp->mWifi->load.noise = noise;
	cp->mWifi->load.mutexChannelLoad.unlock();
	if(PRINT_STATS) {
		cout << "time since last check: " << total_time_diff << endl;
		cout << "busy time since last check: " << busy_time_diff << endl;
		cout << "Calculated Load: " << load << endl;
		cout << "-----------------------" << endl;
	}
	cout << "Received netlink message exit" << endl;
        */
	return NL_SKIP;
}

void PktStatsCollector::probe() {
	cout << "Started probing " << endl;
	while (true) {
		sendNl80211(NL80211_CMD_FLUSH_STATS, &mWifi->ifindex,
				sizeof(mWifi->ifindex), NL80211_ATTR_IFINDEX, NL_AUTO_SEQ,
				NLM_F_DUMP);
		nl_recvmsgs_default(mSocket);
	//	break;
		sleep(1);
	}
	cout << "Exit probing " << endl;
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
		cout << "Prober: Error creating generic netlink header for message." << endl;
		return -1;
	}

	// add message attributes (=payload)
	if (length > 0) {
		ret = nla_put(msg, attrType, length, payload);
		if (ret < 0) {
			cout << "Prober: Error message add attribute: " << nl_geterror(ret) << endl;
			return ret;
		}
	}

	ret = nl_send_auto(mSocket, msg);
	if (ret < 0) {
		cout << "Prober: Error sending message. Error code: " << nl_geterror(ret) << endl;
	}
	nlmsg_free(msg);

	return ret;
}

double PktStatsCollector::getChannelLoad() {
	float load;
	mWifi->load.mutexChannelLoad.lock();
	load = mWifi->load.load;
	mWifi->load.mutexChannelLoad.unlock();
	return (double) load;
}

int main() {
	PktStatsCollector cp("wlan1-ath9k");
	cp.init();

	return 0;
}
