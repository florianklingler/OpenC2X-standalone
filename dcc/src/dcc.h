#ifndef DCC_H_
#define DCC_H_

#include "RingBuffer.h"
#include "LeakyBucket.h"
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <utility/CommunicationReceiver.h>
#include <utility/CommunicationSender.h>
#include <buffers/build/wrapper.pb.h>


class DCC {
public:
	DCC();
	~DCC();

	void init();
	void receiveFromCa();
	void receiveFromDen();
	void receiveFromHw();

	void setCurrentState(int state);
	double simulateChannelLoad();
	void measureChannel(const boost::system::error_code &ec);
	void updateState(const boost::system::error_code &ec);
	void addToken(const boost::system::error_code& e);
	void sendQueuedPackets();

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
	boost::asio::deadline_timer* mTimerAddToken;


	int mCurrentState;

	default_random_engine mRandNumberGen;
	bernoulli_distribution mBernoulli;
	uniform_real_distribution<double> mUniform;

	RingBuffer<double> mChannelLoadInTimeUp;
	RingBuffer<double> mChannelLoadInTimeDown;


	LeakyBucket<wrapperPackage::WRAPPER>* mBucketBE;
};
#endif
