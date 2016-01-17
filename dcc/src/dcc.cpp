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

	mChannelLoadInTimeUp.reset(mConfig.NDL_timeUp / mConfig.DCC_measure_interval_Tm);		//1
	mChannelLoadInTimeDown.reset(mConfig.NDL_timeDown / mConfig.DCC_measure_interval_Tm);	//5
	initialize_States(mConfig.NDL_numActiveState);
	mCurrentStateId = STATE_UNDEF;
	setCurrentState(STATE_RELAXED);

	initialize_LeakyBuckets();

	mTimerMeasure = new boost::asio::deadline_timer(mIoService, boost::posix_time::millisec(mConfig.DCC_measure_interval_Tm*1000));	//1000
	mTimerStateUpdate = new boost::asio::deadline_timer(mIoService, boost::posix_time::millisec(mConfig.NDL_minDccSampling*1000));	//1000
//	mTimerAddTokenVI = new boost::asio::deadline_timer(mIoService, boost::posix_time::millisec(currentPacketInterval(Channels::AC_VI)*1000.00));	//40
//	mTimerAddTokenVO = new boost::asio::deadline_timer(mIoService, boost::posix_time::millisec(currentPacketInterval(Channels::AC_VO)*1000.00));	//40
	mTimerAddTokenBE = new boost::asio::deadline_timer(mIoService, boost::posix_time::millisec(currentPacketInterval(Channels::AC_BE)*1000.00));	//40
//	mTimerAddTokenBK = new boost::asio::deadline_timer(mIoService, boost::posix_time::millisec(currentPacketInterval(Channels::AC_BK)*1000.00));	//40
}

DCC::~DCC() {
	mThreadReceiveFromCa->join();
	mThreadReceiveFromDen->join();
	mThreadReceiveFromHw->join();

	mTimerMeasure->cancel();
	mTimerStateUpdate->cancel();
//	mTimerAddTokenVI->cancel();
//	mTimerAddTokenVO->cancel();
	mTimerAddTokenBE->cancel();
//	mTimerAddTokenBK->cancel();


	//delete mBucketBE;	TODO: fix
}

void DCC::init() {
	mThreadReceiveFromCa = new boost::thread(&DCC::receiveFromCa, this);
	mThreadReceiveFromDen = new boost::thread(&DCC::receiveFromDen, this);
	mThreadReceiveFromHw = new boost::thread(&DCC::receiveFromHw, this);

	mTimerMeasure->async_wait(boost::bind(&DCC::measureChannel, this, boost::asio::placeholders::error));
	mTimerStateUpdate->async_wait(boost::bind(&DCC::updateState, this, boost::asio::placeholders::error));
//	mTimerAddTokenVI->async_wait(boost::bind(&DCC::addToken, this, boost::asio::placeholders::error, Channels::AC_VI, mBucketVI, mTimerAddTokenVI));
//	mTimerAddTokenVO->async_wait(boost::bind(&DCC::addToken, this, boost::asio::placeholders::error, Channels::AC_VO, mBucketVO, mTimerAddTokenVO));
	mTimerAddTokenBE->async_wait(boost::bind(&DCC::addToken, this, boost::asio::placeholders::error, Channels::AC_BE, mBucketBE, mTimerAddTokenBE));
//	mTimerAddTokenBK->async_wait(boost::bind(&DCC::addToken, this, boost::asio::placeholders::error, Channels::AC_BK, mBucketBK, mTimerAddTokenBK));
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
			sendQueuedPackets(mBucketBE);
		}
	}
}

void DCC::receiveFromDen() {
	string envelope;		//envelope
	string byteMessage;		//byte string (serialized WRAPPER)
	wrapperPackage::WRAPPER wrapper;	//deserialized WRAPPER

	while (1) {
		pair<string, string> received = mReceiverFromDen->receive();
		envelope = received.first;
		byteMessage = received.second;

		//processing...
		cout << "received new DENM and enqueue to BE" << endl;

		wrapper.ParseFromString(byteMessage);		//deserialize WRAPPER
		int64_t nowTime = chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1);
		mBucketBE->flushQueue(nowTime);
		bool enqueued = mBucketBE->enqueue(&wrapper, wrapper.validuntil());
		if (enqueued) {
			cout << "Queue length: " << mBucketBE->getQueuedPackets() << endl;
			sendQueuedPackets(mBucketBE);
		}
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

void DCC::initialize_States(int numActiveStates) {

	// defaults for RELAXED state
	states.insert(std::make_pair(STATE_RELAXED, State(mConfig.stateConfig.find(STATE_RELAXED)->second)));

	// defaults for ACTIVE states
	for(int i=1;i<=numActiveStates;i++) {
		states.insert(std::make_pair(i, State(mConfig.stateConfig.find(i)->second)));
	}

	// defaults for RESTRICTED state
	states.insert(std::make_pair(STATE_RESTRICTED, State(mConfig.stateConfig.find(STATE_RESTRICTED)->second)));
}

void DCC::initialize_LeakyBuckets() {
//	mBucketVI = new LeakyBucket<wrapperPackage::WRAPPER>(mConfig.bucketSize_AC_VI, mConfig.queueSize_AC_VI);
//	mBucketVO = new LeakyBucket<wrapperPackage::WRAPPER>(mConfig.bucketSize_AC_VO, mConfig.queueSize_AC_VO);
	mBucketBE = new LeakyBucket<wrapperPackage::WRAPPER>(mConfig.bucketSize_AC_BE, mConfig.queueSize_AC_BE);	//25, 25
//	mBucketBK = new LeakyBucket<wrapperPackage::WRAPPER>(mConfig.bucketSize_AC_BK, mConfig.queueSize_AC_BK);
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

	mTimerMeasure->expires_at(mTimerMeasure->expires_at() + boost::posix_time::milliseconds(mConfig.DCC_measure_interval_Tm*1000));	//1000
	mTimerMeasure->async_wait(boost::bind(&DCC::measureChannel, this, boost::asio::placeholders::error));
}

void DCC::updateState(const boost::system::error_code &ec) {
	double clMinInTimeUp = mChannelLoadInTimeUp.min();		//minimal channel load during TimeUp-interval
	double clMaxInTimeDown = mChannelLoadInTimeDown.max();	//TimeDown-interval > TimeUp-interval

	if ((mCurrentStateId == STATE_RELAXED) && (clMinInTimeUp >= mConfig.NDL_minChannelLoad))				//0.15
		setCurrentState(STATE_ACTIVE1);
	else if ((mCurrentStateId == STATE_RESTRICTED) && (clMaxInTimeDown < mConfig.NDL_maxChannelLoad))		//0.4
		setCurrentState(STATE_ACTIVE1);
	else if ((mCurrentStateId != STATE_UNDEF && mCurrentStateId != STATE_RELAXED && mCurrentStateId != STATE_RESTRICTED) && (clMaxInTimeDown < mConfig.NDL_minChannelLoad))	//0.15
		setCurrentState(STATE_RELAXED);
	else if ((mCurrentStateId != STATE_UNDEF && mCurrentStateId != STATE_RELAXED && mCurrentStateId != STATE_RESTRICTED) && (clMinInTimeUp >= mConfig.NDL_maxChannelLoad))	//0.4
		setCurrentState(STATE_RESTRICTED);

	mTimerStateUpdate->expires_at(mTimerStateUpdate->expires_at() + boost::posix_time::milliseconds(mConfig.NDL_minDccSampling*1000));	//1000
	mTimerStateUpdate->async_wait(boost::bind(&DCC::updateState, this, boost::asio::placeholders::error));
}

void DCC::setCurrentState(int state) {
	int oldStateId = mCurrentStateId;
	State* oldState = mCurrentState;
	mCurrentStateId = state;
	mCurrentState = &states.find(mCurrentStateId)->second;

	if((oldStateId != STATE_UNDEF) && (mCurrentStateId != STATE_RELAXED) && (mCurrentStateId != STATE_RESTRICTED)) {
			// perform state transition to an active state, set corresponding ref values

			//asDcc
			if(mCurrentState->asDcc.ac[Channels::AC_VI].isRef) mCurrentState->asDcc.ac[Channels::AC_VI].val = oldState->asDcc.ac[Channels::AC_VO].val;
			if(mCurrentState->asDcc.ac[Channels::AC_VO].isRef) mCurrentState->asDcc.ac[Channels::AC_VO].val = oldState->asDcc.ac[Channels::AC_VO].val;
			if(mCurrentState->asDcc.ac[Channels::AC_BE].isRef) mCurrentState->asDcc.ac[Channels::AC_BE].val = oldState->asDcc.ac[Channels::AC_BE].val;
			if(mCurrentState->asDcc.ac[Channels::AC_BK].isRef) mCurrentState->asDcc.ac[Channels::AC_BK].val = oldState->asDcc.ac[Channels::AC_BK].val;

			// asTxPower
			if(mCurrentState->asTxPower.ac[Channels::AC_VI].isRef) mCurrentState->asTxPower.ac[Channels::AC_VI].val = oldState->asTxPower.ac[Channels::AC_VI].val;
			if(mCurrentState->asTxPower.ac[Channels::AC_VO].isRef) mCurrentState->asTxPower.ac[Channels::AC_VO].val = oldState->asTxPower.ac[Channels::AC_VO].val;
			if(mCurrentState->asTxPower.ac[Channels::AC_BE].isRef) mCurrentState->asTxPower.ac[Channels::AC_BE].val = oldState->asTxPower.ac[Channels::AC_BE].val;
			if(mCurrentState->asTxPower.ac[Channels::AC_BK].isRef) mCurrentState->asTxPower.ac[Channels::AC_BK].val = oldState->asTxPower.ac[Channels::AC_BK].val;

			// asPacketInterval
			if(mCurrentState->asPacketInterval.ac[Channels::AC_VI].isRef) mCurrentState->asPacketInterval.ac[Channels::AC_VI].val = oldState->asPacketInterval.ac[Channels::AC_VI].val;
			if(mCurrentState->asPacketInterval.ac[Channels::AC_VO].isRef) mCurrentState->asPacketInterval.ac[Channels::AC_VO].val = oldState->asPacketInterval.ac[Channels::AC_VO].val;
			if(mCurrentState->asPacketInterval.ac[Channels::AC_BE].isRef) mCurrentState->asPacketInterval.ac[Channels::AC_BE].val = oldState->asPacketInterval.ac[Channels::AC_BE].val;
			if(mCurrentState->asPacketInterval.ac[Channels::AC_BK].isRef) mCurrentState->asPacketInterval.ac[Channels::AC_BK].val = oldState->asPacketInterval.ac[Channels::AC_BK].val;

			// asDatarate
			if(mCurrentState->asDatarate.ac[Channels::AC_VI].isRef) mCurrentState->asDatarate.ac[Channels::AC_VI].val = oldState->asDatarate.ac[Channels::AC_VI].val;
			if(mCurrentState->asDatarate.ac[Channels::AC_VO].isRef) mCurrentState->asDatarate.ac[Channels::AC_VO].val = oldState->asDatarate.ac[Channels::AC_VO].val;
			if(mCurrentState->asDatarate.ac[Channels::AC_BE].isRef) mCurrentState->asDatarate.ac[Channels::AC_BE].val = oldState->asDatarate.ac[Channels::AC_BE].val;
			if(mCurrentState->asDatarate.ac[Channels::AC_BK].isRef) mCurrentState->asDatarate.ac[Channels::AC_BK].val = oldState->asDatarate.ac[Channels::AC_BK].val;

			// asCarrierSense
			if(mCurrentState->asCarrierSense.ac[Channels::AC_VI].isRef) mCurrentState->asCarrierSense.ac[Channels::AC_VI].val = oldState->asCarrierSense.ac[Channels::AC_VI].val;
			if(mCurrentState->asCarrierSense.ac[Channels::AC_VO].isRef) mCurrentState->asCarrierSense.ac[Channels::AC_VO].val = oldState->asCarrierSense.ac[Channels::AC_VO].val;
			if(mCurrentState->asCarrierSense.ac[Channels::AC_BE].isRef) mCurrentState->asCarrierSense.ac[Channels::AC_BE].val = oldState->asCarrierSense.ac[Channels::AC_BE].val;
			if(mCurrentState->asCarrierSense.ac[Channels::AC_BK].isRef) mCurrentState->asCarrierSense.ac[Channels::AC_BK].val = oldState->asCarrierSense.ac[Channels::AC_BK].val;
		}

	//TODO: reschedule token timer?

	//print current state
	switch (mCurrentStateId) {
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

void DCC::addToken(const boost::system::error_code& e, Channels::t_access_category& ac,  LeakyBucket<wrapperPackage::WRAPPER>*& bucket, boost::asio::deadline_timer*& timer) {
	int64_t nowTime = chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1);
	bucket->flushQueue(nowTime);												//remove all packets that already expired
	bucket->increment();														//add token
	cout << "Available tokens: " << bucket->availableTokens << endl;
	sendQueuedPackets(bucket);

	timer->expires_at(timer->expires_at() + boost::posix_time::milliseconds(currentPacketInterval(ac)*1000.00));
	timer->async_wait(boost::bind(&DCC::addToken, this, boost::asio::placeholders::error, ac, bucket, timer));
}

void DCC::sendQueuedPackets(LeakyBucket<wrapperPackage::WRAPPER>* bucket) {
	while(wrapperPackage::WRAPPER* wrapper = bucket->dequeue()) {
		int64_t nowTime = chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1);
		if(wrapper->validuntil() >= nowTime) {	//message still valid

			//TODO: setMsgLimits(msgQ);
			string byteMessage;
			wrapper->SerializeToString(&byteMessage);
			mSenderToHw->sendToHw(byteMessage);
			cout << "Send message to HW" << endl;
			cout << "Remaining tokens: " << bucket->availableTokens << endl;
			cout << "Queue length: " << bucket->getQueuedPackets() << endl;
		}
//		else																		//message expired
//			delete wrapper;	TODO:find a fix for that
	}
}

//get parameters for current state

dcc_Mechanism_t DCC::currentDcc(Channels::t_access_category ac) {
	dcc_Mechanism_t currentDcc = mCurrentState->asDcc.ac[ac].val;

	return currentDcc;
}
double DCC::currentTxPower(Channels::t_access_category ac) {
	double currentTxPower = mCurrentState->asTxPower.ac[ac].val;

	return currentTxPower;
}
double DCC::currentPacketInterval(Channels::t_access_category ac) {
	double currentPacketInterval = mCurrentState->asPacketInterval.ac[ac].val;

	cout << " " << currentPacketInterval*1000.00 << endl;
	return currentPacketInterval;
}
double DCC::currentDatarate(Channels::t_access_category ac) {
	double currentDatarate = mCurrentState->asDatarate.ac[ac].val;

	return currentDatarate;
}
double DCC::currentCarrierSense(Channels::t_access_category ac) {
	double currentCarrierSense = mCurrentState->asCarrierSense.ac[ac].val;

	return currentCarrierSense;
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
