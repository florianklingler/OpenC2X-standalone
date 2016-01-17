#ifndef DCC_H_
#define DCC_H_

#include "DccConfig.h"
#include "State.h"
#include "RingBuffer.h"
#include "LeakyBucket.h"
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <utility/CommunicationReceiver.h>
#include <utility/CommunicationSender.h>
#include <buffers/build/wrapper.pb.h>


class DCC {
public:
	DCC(DccConfig &config);
	~DCC();

	void init();
	void receiveFromCa();
	void receiveFromDen();
	void receiveFromHw();

	void initialize_States(int numActiveStates);
	void setCurrentState(int state);
	double simulateChannelLoad();
	void measureChannel(const boost::system::error_code &ec);
	void updateState(const boost::system::error_code &ec);
	void initialize_LeakyBuckets();
	void addToken(const boost::system::error_code& e, Channels::t_access_category& ac, LeakyBucket<wrapperPackage::WRAPPER>*& bucket, boost::asio::deadline_timer*& timer);
	void sendQueuedPackets(LeakyBucket<wrapperPackage::WRAPPER>* bucket);

	dcc_Mechanism_t currentDcc(Channels::t_access_category ac);
	double currentTxPower(Channels::t_access_category ac);
	double currentPacketInterval(Channels::t_access_category ac);
	double currentDatarate(Channels::t_access_category ac);
	double currentCarrierSense(Channels::t_access_category ac);

private:
	CommunicationReceiver* mReceiverFromCa;
	CommunicationReceiver* mReceiverFromDen;
	CommunicationReceiver* mReceiverFromHw;
	CommunicationSender* mSenderToHw;
	CommunicationSender* mSenderToServices;

	boost::thread* mThreadReceiveFromCa;
	boost::thread* mThreadReceiveFromDen;
	boost::thread* mThreadReceiveFromHw;

	boost::asio::io_service mIoService;
	boost::asio::deadline_timer* mTimerMeasure;
	boost::asio::deadline_timer* mTimerStateUpdate;
//	boost::asio::deadline_timer* mTimerAddTokenVI;
//	boost::asio::deadline_timer* mTimerAddTokenVO;
	boost::asio::deadline_timer* mTimerAddTokenBE;
//	boost::asio::deadline_timer* mTimerAddTokenBK;

	default_random_engine mRandNumberGen;
	bernoulli_distribution mBernoulli;
	uniform_real_distribution<double> mUniform;

	RingBuffer<double> mChannelLoadInTimeUp;
	RingBuffer<double> mChannelLoadInTimeDown;


//	LeakyBucket<wrapperPackage::WRAPPER>* mBucketVI;
//	LeakyBucket<wrapperPackage::WRAPPER>* mBucketVO;
	LeakyBucket<wrapperPackage::WRAPPER>* mBucketBE;
//	LeakyBucket<wrapperPackage::WRAPPER>* mBucketBK;

	DccConfig mConfig;

	States states;
	int mCurrentStateId;
	State* mCurrentState;
};
#endif
