#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include "ChannelProber.h"
#include <unistd.h>
#include <string>
#include <iostream>
#include <linux/nl80211.h>
#include <net/if.h>

#define PRINT_STATS 0

using namespace std;

int ChannelProber::mNl80211Id;

ChannelProber::ChannelProber() {
	mChannelLoadProbingPeriod = 1000;
	mTotalWifi = 1;
}

ChannelProber::~ChannelProber() {
	mThreadProbe->join();
	delete mThreadProbe;

	free(wifi);

	nl_socket_free(mSocket);
	mSocket = NULL;
}

void ChannelProber::init() {
	int ret;
	wifi = (netinterface*) calloc(mTotalWifi, sizeof(netinterface));
	for (int i = 0; i < mTotalWifi; i++) {
		wifi[i].ifindex = if_nametoindex("wlan0");
		cout << "if index: " << wifi[i].ifindex << endl;
		if (wifi[i].ifindex == 0) {
			cout << "error getting network interface. errno: " << errno << endl;
			wifi[i].ifindex = -1;
			wifi[i].channel = -1;
		}
		wifi[i].load.load = -1;
		wifi[i].load.total_time_last = 0;
		wifi[i].load.busy_time_last = 0;
	}

	// initialize socket now
	mSocket = nl_socket_alloc();
	if (mSocket == NULL) {
		cout << "socket_alloc error" << endl;
		return;
	}

	// disable sequence number checking. We want to receive notifications.
	nl_socket_disable_seq_check(mSocket);

	ret = nl_socket_modify_cb(mSocket, NL_CB_VALID, NL_CB_CUSTOM,
			ChannelProber::receivedNetlinkMsg, this);
	cout << "NL Socket Callback ret: " << ret << endl;

	// connect socket. Protocol: generic netlink
	ret = genl_connect(mSocket);
	if (ret != 0) {
		cout << "Connection to Generic Netlink Socket failed! err: "
				<< nl_geterror(ret) << endl;
		return;
	}

	// resolve mNl80211Id
	mNl80211Id = genl_ctrl_resolve(mSocket, "nl80211");
	if (mNl80211Id < 0) {
		cout << "error code getting nl80211 id: " << nl_geterror(mNl80211Id)
				<< endl;
		return;
	}

	mThreadProbe = new boost::thread(&ChannelProber::probe, this);
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
		fprintf(stderr, "survey data missing!\n");
		return NL_SKIP;
	}
	static struct nla_policy survey_policy[NL80211_SURVEY_INFO_MAX + 1] = { };

	if (nla_parse_nested(sinfo, NL80211_SURVEY_INFO_MAX,
			tb[NL80211_ATTR_SURVEY_INFO], survey_policy)) {
		fprintf(stderr, "failed to parse nested attributes!\n");
		return NL_SKIP;
	}

	// If this info is not about the channel in use, then skip
	if (!sinfo[NL80211_SURVEY_INFO_IN_USE]) {
		return NL_SKIP;
	}

	// Current channel in use
	if (sinfo[NL80211_SURVEY_INFO_FREQUENCY]) {
		channel = nla_get_u32(sinfo[NL80211_SURVEY_INFO_FREQUENCY]);
		if(PRINT_STATS) {
			cout << "\tfrequency:\t\t\t" << channel << " MHz" << endl;
		}
	}

	// Noise
	if (sinfo[NL80211_SURVEY_INFO_NOISE]) {
		noise = (int8_t) nla_get_u8(sinfo[NL80211_SURVEY_INFO_NOISE]);
		// cout<<"\tnoise:\t\t\t\t"<< noise <<" dBm"<<endl;
	}

	// Total active time
	if (sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME]) {
		total_time = nla_get_u64(sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME]);
		if(PRINT_STATS) {
			cout << "\tchannel active time:\t\t" << total_time << " ms" << endl;
		}
	}

	// Total busy time
	if (sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME_BUSY]) {
		busy_time = nla_get_u64(sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME_BUSY]);
		if(PRINT_STATS) {
			cout << "\tchannel busy time:\t\t" << busy_time << " ms" << endl;
		}
	}

	// Do we need info about extension channel?
	if (sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME_EXT_BUSY]) {
		// cout << "\textension channel busy time:\t"
		//		<< nla_get_u64(sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME_EXT_BUSY])
		//		<< " ms" << endl;
	}

	// Total receiving time
	if (sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME_RX]) {
		if(PRINT_STATS) {
			cout << "\tchannel receive time:\t\t"
				<< nla_get_u64(sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME_RX])
				<< " ms" << endl;
		}
	}

	// Total transmitting time
	if (sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME_TX]) {
		if(PRINT_STATS) {
			cout << "\tchannel transmit time:\t\t"
				<< nla_get_u64(sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME_TX])
				<< " ms" << endl;
		}
	}

	// Update statistics
	uint64_t busy_time_diff = busy_time - cp->wifi[0].load.busy_time_last;
	uint64_t total_time_diff = total_time - cp->wifi[0].load.total_time_last;
	double load = ((100 * busy_time_diff) / total_time_diff) / 100.0;
	cp->wifi[0].load.mutex_channelLoad.lock();
	cp->wifi[0].load.load = load;
	cp->wifi[0].load.total_time_last = total_time;
	cp->wifi[0].load.busy_time_last = busy_time;
	cp->wifi[0].load.noise = noise;
	cp->wifi[0].load.mutex_channelLoad.unlock();
	if(PRINT_STATS) {
		cout << "time since last check: " << total_time_diff << endl;
		cout << "busy time since last check: " << busy_time_diff << endl;
		cout << "Calculated Load: " << load << endl;
		cout << "-----------------------" << endl;
	}
	return NL_SKIP;
}

void ChannelProber::probe() {
	cout << "started probing the channel every 1 sec" << endl;
	while (true) {
		sendNl80211(NL80211_CMD_GET_SURVEY, &wifi[0].ifindex,
				sizeof(wifi[0].ifindex), NL80211_ATTR_IFINDEX, NL_AUTO_SEQ,
				NLM_F_DUMP);
		nl_recvmsgs_default(mSocket);
		sleep(1);
	}
}

int ChannelProber::sendNl80211(uint8_t msgCmd, void *payload,
		unsigned int pLength, int payloadType, unsigned int seq, int flags) {
	return send(msgCmd, payload, pLength, payloadType, seq, mNl80211Id, flags,
			0x01);
}

int ChannelProber::send(uint8_t msgCmd, void *payload, unsigned int pLength,
		int attrType, unsigned int seq, int protocol_id, int flags,
		uint8_t protocol_version) {
	int ret = 0;

	// create a new Netlink message
	nl_msg *msg = nlmsg_alloc();

	// create message header
	if (genlmsg_put(msg, 0, seq, protocol_id, 0, flags, msgCmd,
			protocol_version) == NULL) {
		cout << "ChannelProber: Error creating generic netlink header for message." << endl;
		return -1;
	}

	// add message attributes (=payload)
	if (pLength > 0) {
		ret = nla_put(msg, attrType, pLength, payload);
		if (ret < 0) {
			cout << "Error message add attribute: " << nl_geterror(ret) << endl;
			return ret;
		}
	}

	ret = nl_send_auto(mSocket, msg);
	if (ret < 0) {
		cout << "Error sending message. Error code: " << nl_geterror(ret) << endl;
	} else {
		if(PRINT_STATS) {
			cout << ret << " bytes sent" << endl;
		}
	}
	nlmsg_free(msg);

	return ret;
}

double ChannelProber::getChannelLoad() {
	float load;
	wifi[0].load.mutex_channelLoad.lock();
	load = wifi[0].load.load;
	wifi[0].load.mutex_channelLoad.unlock();
	return (double) load;
}

int _main() {
	ChannelProber prober;
	prober.init();

	return 0;
}
