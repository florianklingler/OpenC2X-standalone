#ifndef DCC_H_
#define DCC_H_

#include "DccConfig.h"
#include "State.h"
#include "RingBuffer.h"
#include "LeakyBucket.h"
#include <mutex>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <utility/CommunicationReceiver.h>
#include <utility/CommunicationSender.h>
#include <utility/LoggingUtility.h>
#include <buffers/build/data.pb.h>
#include <buffers/build/dccInfo.pb.h>
#include "SendToHardwareViaMAC.h"
#include "ReceiveFromHardwareViaMAC.h"
#include "ChannelProber.h"
#include <random>


using namespace std;

class DCC {
public:
	DCC(DccConfig &config);
	~DCC();

	void init();
	void receiveFromCa();
	void receiveFromDen();
	void receiveFromHw();
	void sendDccInfo(const boost::system::error_code& ec);

	void initStates(int numActiveStates);
	void setCurrentState(int state);
	double simulateChannelLoad();
	void measureChannel(const boost::system::error_code& ec);
	void updateState(const boost::system::error_code& ec);
	void initLeakyBuckets();
	void addToken(const boost::system::error_code& ec, Channels::t_access_category ac);
	void rescheduleAddToken(Channels::t_access_category ac);
	void sendQueuedPackets(Channels::t_access_category ac);
	void setMessageLimits(dataPackage::DATA* data);

	dcc_Mechanism_t currentDcc(Channels::t_access_category ac);
	double currentTxPower(Channels::t_access_category ac);
	double currentTokenInterval(Channels::t_access_category ac);
	double currentDatarate(Channels::t_access_category ac);
	double currentCarrierSense(Channels::t_access_category ac);

private:
	const Channels::t_access_category mAccessCategories[4] = {Channels::AC_VI, Channels::AC_VO, Channels::AC_BE, Channels::AC_BK};	//all used ACs

	CommunicationReceiver* mReceiverFromCa;
	CommunicationReceiver* mReceiverFromDen;
	CommunicationSender* mSenderToServices;
	CommunicationSender* mSenderToLdm;

	LoggingUtility* mLogger;

	boost::thread* mThreadReceiveFromCa;
	boost::thread* mThreadReceiveFromDen;
	boost::thread* mThreadReceiveFromHw;

	SendToHardwareViaMAC* mSenderToHw;
	ReceiveFromHardwareViaMAC* mReceiverFromHw;

	boost::asio::io_service mIoService;
	boost::asio::io_service::strand mStrand;
	boost::asio::deadline_timer* mTimerMeasure;
	boost::asio::deadline_timer* mTimerStateUpdate;
	boost::asio::deadline_timer* mTimerDccInfo;

	map<Channels::t_access_category, boost::asio::deadline_timer*> mTimerAddToken;	//timers for all four ACs

	default_random_engine mRandNumberGen;
	bernoulli_distribution mBernoulli;
	uniform_real_distribution<double> mUniform;

	RingBuffer<double> mChannelLoadInTimeUp;	//holds the recent channel load measurements (influences state changes)
	RingBuffer<double> mChannelLoadInTimeDown;

	map<Channels::t_access_category, LeakyBucket<dataPackage::DATA>*> mBucket;	//LeakyBuckets for all four ACs

	DccConfig mConfig;

	States states;			//map of all states
	int mCurrentStateId;
	State* mCurrentState;

	ChannelProber* mChannelProber;
	double mChannelLoad;

	mutex mMutexLastTokenAt;
	map<Channels::t_access_category, bool> mAddedFirstToken;					//was any token added in this state, yet?
	map<Channels::t_access_category, boost::posix_time::ptime> mLastTokenAt;	//when was the last token added in this state
};
#endif
