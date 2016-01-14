#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include "dcc.h"
#include <unistd.h>
#include <string>
#include <iostream>
#include <random>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace std;

INITIALIZE_EASYLOGGINGPP

const int STATE_UNDEF = -1;
const int STATE_RELAXED = 0;
const int STATE_ACTIVE1 = 1;
const int STATE_RESTRICTED = 2;		//TODO: has to be adjusted when using more than one active state


DCC::DCC() {
	mReceiverFromCa = new CommunicationReceiver("Dcc", "6666", "CAM");
	mReceiverFromDen = new CommunicationReceiver("Dcc", "7777", "DENM");
	mReceiverFromHw = new CommunicationReceiver("Dcc", "4444", "");
	mSenderToHw = new CommunicationSender("Dcc", "4444");
	mSenderToServices = new CommunicationSender("Dcc", "5555");

	mRandNumberGen = default_random_engine(0);
	mBernoulli = bernoulli_distribution(0);
	mUniform = uniform_real_distribution<double>(-0.1, 0.1);

	mBucketBE = new LeakyBucket<wrapperPackage::WRAPPER>(25, 25);	//bucket size, queue size

	mChannelLoadInTimeUp.reset(1);		//NDL_timeUp / DCC_measureInterval
	mChannelLoadInTimeDown.reset(5);	//NDL_timeDown / DCC_measureInterval

	mCurrentState = STATE_UNDEF;
	setCurrentState(STATE_RELAXED);

	mTimerMeasure = new boost::asio::deadline_timer(mIoService, boost::posix_time::millisec(1000));
	mTimerStateUpdate = new boost::asio::deadline_timer(mIoService, boost::posix_time::millisec(1000));
	mTimerAddToken = new boost::asio::deadline_timer(mIoService, boost::posix_time::millisec(2000));	//TODO: adjust interval based on state and AC
}

DCC::~DCC() {
	mThreadReceiveFromCa->join();
	mThreadReceiveFromDen->join();
	mThreadReceiveFromHw->join();

	//delete mBucketBE;
}

void DCC::init() {
	mThreadReceiveFromCa = new boost::thread(&DCC::receiveFromCa, this);
	mThreadReceiveFromDen = new boost::thread(&DCC::receiveFromDen, this);
	mThreadReceiveFromHw = new boost::thread(&DCC::receiveFromHw, this);

	mTimerMeasure->async_wait(boost::bind(&DCC::measureChannel, this, boost::asio::placeholders::error));
	mTimerStateUpdate->async_wait(boost::bind(&DCC::updateState, this, boost::asio::placeholders::error));
	mTimerAddToken->async_wait(boost::bind(&DCC::addToken, this, boost::asio::placeholders::error));
	mIoService.run();
}


//////////////////////////////////////////
//Send & receive

void DCC::receiveFromCa() {
	string envelope;					//envelope
	string byteMessage;					//byte string (serialized WRAPPER)
	wrapperPackage::WRAPPER wrapper;	//deserialized WRAPPER

	while (1) {
		pair<string, string> received = mReceiverFromCa->receive();
		envelope = received.first;
		byteMessage = received.second;

		//processing...
		cout << "received new CAM and enqueue to BE" << endl;

		wrapper.ParseFromString(byteMessage);		//deserialize WRAPPER
		int64_t nowTime = chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1);
		mBucketBE->flushQueue(nowTime);
		bool enqueued = mBucketBE->enqueue(&wrapper, wrapper.validuntil());
		if (enqueued) {
			cout << "Queue length: " << mBucketBE->getQueuedPackets() << endl;
			sendQueuedPackets();
		}
	}
}

void DCC::receiveFromDen() {
	string envelope;		//envelope
	string byteMessage;		//byte string (serialized WRAPPER)
	while (1) {
		pair<string, string> received = mReceiverFromDen->receive();
		envelope = received.first;
		byteMessage = received.second;

		//processing...
		cout << "received new DENM and forward to HW" << endl;
		mSenderToHw->sendToHw(byteMessage);
	}
}

void DCC::receiveFromHw() {
	string byteMessage;		//byte string (serialized message)
	wrapperPackage::WRAPPER wrapper;

	while (1) {
		byteMessage = mReceiverFromHw->receiveFromHw();		//receive serialized WRAPPER
		wrapper.ParseFromString(byteMessage);				//deserialize WRAPPER

		//processing...
		cout << "forward message from HW to services" << endl;
		switch(wrapper.type()) {							//send serialized WRAPPER to corresponding module
			case wrapperPackage::WRAPPER_Type_CAM: 		mSenderToServices->send("CAM", byteMessage);	break;
			case wrapperPackage::WRAPPER_Type_DENM:		mSenderToServices->send("DENM", byteMessage);	break;
			default:	break;
		}

	}
}


//////////////////////////////////////////
//Decentralized congestion control

void DCC::setCurrentState(int state) {
	//int oldState = mCurrentState;	//TODO: used later for states (refs)
	mCurrentState = state;

	switch (mCurrentState) {
	case STATE_UNDEF:
		cout << "Current state: Undefined" << endl;
		break;
	case STATE_RELAXED:
		cout << "Current state: Relaxed" << endl;
		break;
	case STATE_ACTIVE1:
		cout << "Current state: Active1" << endl;
		break;
	case STATE_RESTRICTED:
		cout << "Current state: Restricted" << endl;
		break;
	default: break;
	}
}

double DCC::simulateChannelLoad() {	//just for testing/simulation
	double pCurr = mBernoulli.p();
	double pNew = pCurr + mUniform(mRandNumberGen);
	pNew = min(0.5, max(0.05, pNew));

	mBernoulli = bernoulli_distribution(pNew);

	double sum = 0;

	for (int i=0; i<100000; i++) {	//100000 probings correspond to a probing interval of 10 us
		double r = mBernoulli(mRandNumberGen);
		sum += min(1.0, max(0.0, r));
	}

	return sum / 100000.0;	//avg of probings to avoid rapid/drastic changes in channel load
}

void DCC::measureChannel(const boost::system::error_code &ec) {
	double channelLoad = simulateChannelLoad();

	mChannelLoadInTimeUp.insert(channelLoad);	//add to RingBuffer
	mChannelLoadInTimeDown.insert(channelLoad);

	mTimerMeasure->expires_from_now(boost::posix_time::millisec(1000));	//DCC_measureInterval
	mTimerMeasure->async_wait(boost::bind(&DCC::measureChannel, this, boost::asio::placeholders::error));
}

void DCC::updateState(const boost::system::error_code &ec) {
	double clMinInTimeUp = mChannelLoadInTimeUp.min();		//minimal channel load during TimeUp-interval
	double clMaxInTimeDown = mChannelLoadInTimeDown.max();	//TimeDown-interval > TimeUp-interval

	if ((mCurrentState == STATE_RELAXED) && (clMinInTimeUp >= 0.15))			//NDL_minChannelLoad
		setCurrentState(STATE_ACTIVE1);
	else if ((mCurrentState == STATE_RESTRICTED) && (clMaxInTimeDown < 0.4))	//NDL_maxChannelLoad
		setCurrentState(STATE_ACTIVE1);
	else if ((mCurrentState != STATE_UNDEF && mCurrentState != STATE_RELAXED && mCurrentState != STATE_RESTRICTED) && (clMaxInTimeDown < 0.15))	//NDL_minChannelLoad
		setCurrentState(STATE_RELAXED);
	else if ((mCurrentState != STATE_UNDEF && mCurrentState != STATE_RELAXED && mCurrentState != STATE_RESTRICTED) && (clMinInTimeUp >= 0.4))	//NDL_maxChannelLoad
		setCurrentState(STATE_RESTRICTED);

	mTimerStateUpdate->expires_from_now(boost::posix_time::millisec(1000));		//NDL_minDccSampling
	mTimerStateUpdate->async_wait(boost::bind(&DCC::updateState, this, boost::asio::placeholders::error));
}

void DCC::addToken(const boost::system::error_code& e) {
	int64_t nowTime = chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1);
	mBucketBE->flushQueue(nowTime);												//remove all packets that already expired
	mBucketBE->increment();														//add token
	cout << "Available tokens: " << mBucketBE->availableTokens << endl;
	sendQueuedPackets();

	mTimerAddToken->expires_from_now(boost::posix_time::millisec(2000));			//TODO: adjust interval based on state and AC
	mTimerAddToken->async_wait(boost::bind(&DCC::addToken, this, boost::asio::placeholders::error));
}

void DCC::sendQueuedPackets() {
	bool sent = false;
	while(wrapperPackage::WRAPPER* wrapper = mBucketBE->dequeue()) {
		int64_t nowTime = chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1);
		if(wrapper->validuntil() >= nowTime) {	//message still valid

			//setMsgLimits(msgQ);
			string byteMessage;
			wrapper->SerializeToString(&byteMessage);
			mSenderToHw->sendToHw(byteMessage);
			sent = true;
			cout << "Send CAM to HW" << endl;
			cout << "Remaining tokens: " << mBucketBE->availableTokens << endl;
			cout << "Queue length: " << mBucketBE->getQueuedPackets() << endl;
		}
//		else																		//message expired
//			delete wrapper;
	}
}



int main() {
	DCC dcc;
	dcc.init();

	return EXIT_SUCCESS;
}
