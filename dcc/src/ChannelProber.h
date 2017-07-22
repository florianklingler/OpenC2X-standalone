// This file is part of OpenC2X.
//
// OpenC2X is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenC2X is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OpenC2X.  If not, see <http://www.gnu.org/licenses/>.
//
// Authors:
// Sven Laux <slaux@mail.uni-paderborn.de>
// Gurjashan Singh Pannu <gurjashan.pannu@ccs-labs.org>
// Stefan Schneider <stefan.schneider@ccs-labs.org>
// Jan Tiemann <janhentie@web.de>


#ifndef CHANNELPROBER_H_
#define CHANNELPROBER_H_

/**
 * @addtogroup dccHardware
 * @{
 */

#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <common/utility/LoggingUtility.h>
#include <netlink/netlink-kernel.h>
#include <netlink/socket.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/mngt.h>

/**
 * ChannelProber probes the Ath9k NIC for the channel busy ratio (CBR) periodically via netlink.
 * The measured channel busy ratio corresponds to previous 1 second.
 */
class ChannelProber {
public:
	/**
	 * Constructor.
	 * @param ifname WLan interface name
	 * @param probeInterval time interval between two consecutive probes
	 * @param io boost io service
	 * @param expNo experiment number
	 */
	ChannelProber(std::string ifname, double probeInterval, boost::asio::io_service* io, int expNo, std::string loggingConf, std::string statisticConf);
	virtual ~ChannelProber();

	/**
	 * Initializes channel prober.
	 * Allocate a netlink socket, connect to it and start probing.
	 */
	void init();

	/**
	 * Starts channel probing.
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
	 * @param protocolVersion protocol version
	 * @return 0 if success, < 0 otherwise.
	 */
	int send(uint8_t msgCmd, void *payload, unsigned int length, int attrType, unsigned int seq, int protocolId, int flags = 0, uint8_t protocolVersion = 0x01);

	/**
	 * Get the measured channel load.
	 * @return The channel load measured in last 1 second.
	 */
	double getChannelLoad();

	/** Receives the netlink message from the kernel.
	 * The received message is parsed and the current information about the channel utilization is saved in the channelload struct.
	 * @param msg Received netlink message corresponding to the probe
	 * @param arg Pointer to ChannelProber
	 * @return 1
	 */
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
	boost::asio::io_service* mIoService;
	boost::asio::deadline_timer* mTimer;
};

/**
 * @}
 */

#endif
