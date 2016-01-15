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

DCC::DCC(DccConfig &config) {
	string module = "Dcc";
	mConfig = config;
	mReceiverFromCa = new CommunicationReceiver(module, "6666", "CAM");
	mReceiverFromDen = new CommunicationReceiver(module, "7777", "DENM");
	mReceiverFromHw = new CommunicationReceiver(module, "4444", "");
	mSenderToHw = new CommunicationSender(module, "4444");
	mSenderToServices = new CommunicationSender(module, "5555");

	mRandNumberGen = default_random_engine(0);
	mBernoulli = bernoulli_distribution(0);
	mUniform = uniform_real_distribution<double>(-0.1, 0.1);

	mBucketBE = new LeakyBucket<wrapperPackage::WRAPPER>(mConfig.bucketSize_AC_BE, mConfig.queueSize_AC_BE);	//25, 25

	mChannelLoadInTimeUp.reset(mConfig.NDL_timeUp / mConfig.DCC_measure_interval_Tm);		//1
	mChannelLoadInTimeDown.reset(mConfig.NDL_timeDown / mConfig.DCC_measure_interval_Tm);	//5
	mCurrentState = STATE_UNDEF;
	setCurrentState(STATE_RELAXED);

	mTimerMeasure = new boost::asio::deadline_timer(mIoService, boost::posix_time::millisec(mConfig.DCC_measure_interval_Tm*1000));	//1000
	mTimerStateUpdate = new boost::asio::deadline_timer(mIoService, boost::posix_time::millisec(mConfig.NDL_minDccSampling*1000));	//1000
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

	mTimerMeasure->expires_from_now(boost::posix_time::millisec(mConfig.DCC_measure_interval_Tm*1000));	//1000
	mTimerMeasure->async_wait(boost::bind(&DCC::measureChannel, this, boost::asio::placeholders::error));
}

void DCC::updateState(const boost::system::error_code &ec) {
	double clMinInTimeUp = mChannelLoadInTimeUp.min();		//minimal channel load during TimeUp-interval
	double clMaxInTimeDown = mChannelLoadInTimeDown.max();	//TimeDown-interval > TimeUp-interval

	if ((mCurrentState == STATE_RELAXED) && (clMinInTimeUp >= mConfig.NDL_minChannelLoad))				//0.15
		setCurrentState(STATE_ACTIVE1);
	else if ((mCurrentState == STATE_RESTRICTED) && (clMaxInTimeDown < mConfig.NDL_maxChannelLoad))		//0.4
		setCurrentState(STATE_ACTIVE1);
	else if ((mCurrentState != STATE_UNDEF && mCurrentState != STATE_RELAXED && mCurrentState != STATE_RESTRICTED) && (clMaxInTimeDown < mConfig.NDL_minChannelLoad))	//0.15
		setCurrentState(STATE_RELAXED);
	else if ((mCurrentState != STATE_UNDEF && mCurrentState != STATE_RELAXED && mCurrentState != STATE_RESTRICTED) && (clMinInTimeUp >= mConfig.NDL_maxChannelLoad))	//0.4
		setCurrentState(STATE_RESTRICTED);

	mTimerStateUpdate->expires_from_now(boost::posix_time::millisec(mConfig.NDL_minDccSampling*1000));		//1000
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
	DccConfig config;
	try {
		// TODO: set proper path to config.xml
		// Right now, pwd is dcc/Debug while running dcc
		config.load_base_Parameters("../src/config.xml");
		config.load_NDL_Parameters();
	} catch (std::exception &e) {
		cerr << "Error while loading config.xml: " << e.what() << endl << flush;
		return EXIT_FAILURE;
	}
	DCC dcc(config);
	dcc.init();

	return EXIT_SUCCESS;
}
