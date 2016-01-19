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
	initStates(mConfig.NDL_numActiveState);
	mCurrentStateId = STATE_UNDEF;
	setCurrentState(STATE_RELAXED);

	initLeakyBuckets();

	mTimerMeasure = new boost::asio::deadline_timer(mIoService, boost::posix_time::millisec(mConfig.DCC_measure_interval_Tm*1000));	//1000
	mTimerStateUpdate = new boost::asio::deadline_timer(mIoService, boost::posix_time::millisec(mConfig.NDL_minDccSampling*1000));	//1000

	for (Channels::t_access_category accessCategory : mAccessCategories) {	//create and map the timers for the four ACs
		mTimerAddToken.insert(make_pair(accessCategory, new boost::asio::deadline_timer(mIoService, boost::posix_time::millisec(currentTokenInterval(accessCategory)*1000.00))));
		mAddedFirstToken[accessCategory] = false;
	}
}

DCC::~DCC() {
	mThreadReceiveFromCa->join();
	mThreadReceiveFromDen->join();
	mThreadReceiveFromHw->join();

	mTimerMeasure->cancel();
	mTimerStateUpdate->cancel();

	for (Channels::t_access_category accessCategory : mAccessCategories) {	//for each AC
		mTimerAddToken[accessCategory]->cancel();
	}


//	delete mBucketBE;	//TODO: correct deletes for everything
}

void DCC::init() {
	mThreadReceiveFromCa = new boost::thread(&DCC::receiveFromCa, this);
	mThreadReceiveFromDen = new boost::thread(&DCC::receiveFromDen, this);
	mThreadReceiveFromHw = new boost::thread(&DCC::receiveFromHw, this);

	//start timers
	mTimerMeasure->async_wait(boost::bind(&DCC::measureChannel, this, boost::asio::placeholders::error));
	mTimerStateUpdate->async_wait(boost::bind(&DCC::updateState, this, boost::asio::placeholders::error));

	for (Channels::t_access_category accessCategory : mAccessCategories) {	//for each AC
		mTimerAddToken[accessCategory]->async_wait(boost::bind(&DCC::addToken, this, boost::asio::placeholders::error, accessCategory));
	}

	mIoService.run();
}

//initializes states and sets default values
void DCC::initStates(int numActiveStates) {
	states.insert(make_pair(STATE_RELAXED, State(mConfig.stateConfig.find(STATE_RELAXED)->second)));		//relaxed state

	for(int i=1;i<=numActiveStates;i++)																		//active states
		states.insert(make_pair(i, State(mConfig.stateConfig.find(i)->second)));

	states.insert(make_pair(STATE_RESTRICTED, State(mConfig.stateConfig.find(STATE_RESTRICTED)->second)));	//restricted states
}

void DCC::initLeakyBuckets() {
	mBucket.insert(make_pair(Channels::AC_VI, new LeakyBucket<wrapperPackage::WRAPPER>(mConfig.bucketSize_AC_VI, mConfig.queueSize_AC_VI)));
	mBucket.insert(make_pair(Channels::AC_VO, new LeakyBucket<wrapperPackage::WRAPPER>(mConfig.bucketSize_AC_VO, mConfig.queueSize_AC_VO)));
	mBucket.insert(make_pair(Channels::AC_BE, new LeakyBucket<wrapperPackage::WRAPPER>(mConfig.bucketSize_AC_BE, mConfig.queueSize_AC_BE)));
	mBucket.insert(make_pair(Channels::AC_BK, new LeakyBucket<wrapperPackage::WRAPPER>(mConfig.bucketSize_AC_BK, mConfig.queueSize_AC_BK)));
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
		cout << "received new CAM -> enqueue to BE" << endl;

		wrapper.ParseFromString(byteMessage);		//deserialize WRAPPER
		int64_t nowTime = chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1);

		Channels::t_access_category ac = (Channels::t_access_category) wrapper.priority();
		mBucket[ac]->flushQueue(nowTime);
		bool enqueued = mBucket[ac]->enqueue(&wrapper, wrapper.validuntil());
		if (enqueued) {
			cout << "Queue length: " << mBucket[ac]->getQueuedPackets() << endl;
			sendQueuedPackets(ac);
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
		cout << "received new DENM -> enqueue at DCC" << endl;

		wrapper.ParseFromString(byteMessage);		//deserialize WRAPPER
		int64_t nowTime = chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1);

		Channels::t_access_category ac = (Channels::t_access_category) wrapper.priority();
		mBucket[ac]->flushQueue(nowTime);
		bool enqueued = mBucket[ac]->enqueue(&wrapper, wrapper.validuntil());
		if (enqueued) {
			cout << "Queue length: " << mBucket[ac]->getQueuedPackets() << endl;
			sendQueuedPackets(ac);
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

//simulates realistic channel load; to be replaced with actual measuremetns
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

//gets the measured (or simulated) channel load and stores it in 2 ring buffers (to decide state changes)
void DCC::measureChannel(const boost::system::error_code &ec) {
	double channelLoad = simulateChannelLoad();

	mChannelLoadInTimeUp.insert(channelLoad);	//add to RingBuffer
	mChannelLoadInTimeDown.insert(channelLoad);

	mTimerMeasure->expires_at(mTimerMeasure->expires_at() + boost::posix_time::milliseconds(mConfig.DCC_measure_interval_Tm*1000));	//1s
	mTimerMeasure->async_wait(boost::bind(&DCC::measureChannel, this, boost::asio::placeholders::error));
}

//updates state according to recent channel load measurements
void DCC::updateState(const boost::system::error_code &ec) {
	double clMinInTimeUp = mChannelLoadInTimeUp.min();		//minimal channel load during TimeUp-interval
	double clMaxInTimeDown = mChannelLoadInTimeDown.max();	//TimeDown-interval > TimeUp-interval

	if ((mCurrentStateId == STATE_RELAXED) && (clMinInTimeUp >= mConfig.NDL_minChannelLoad))				//relaxed -> active1
		setCurrentState(STATE_ACTIVE1);
	else if ((mCurrentStateId == STATE_RESTRICTED) && (clMaxInTimeDown < mConfig.NDL_maxChannelLoad))		//restricted -> active1
		setCurrentState(STATE_ACTIVE1);
	else if ((mCurrentStateId != STATE_UNDEF && mCurrentStateId != STATE_RELAXED && mCurrentStateId != STATE_RESTRICTED) && (clMaxInTimeDown < mConfig.NDL_minChannelLoad))	//active -> relaxed
		setCurrentState(STATE_RELAXED);
	else if ((mCurrentStateId != STATE_UNDEF && mCurrentStateId != STATE_RELAXED && mCurrentStateId != STATE_RESTRICTED) && (clMinInTimeUp >= mConfig.NDL_maxChannelLoad))	//active -> restricted
		setCurrentState(STATE_RESTRICTED);

	mTimerStateUpdate->expires_at(mTimerStateUpdate->expires_at() + boost::posix_time::milliseconds(mConfig.NDL_minDccSampling*1000));	//1s
	mTimerStateUpdate->async_wait(boost::bind(&DCC::updateState, this, boost::asio::placeholders::error));
}

//sets the current state and updates corresponding characteristics and intervals
void DCC::setCurrentState(int state) {
	int oldStateId = mCurrentStateId;
	State* oldState = mCurrentState;
	mCurrentStateId = state;
	mCurrentState = &states.find(mCurrentStateId)->second;

	//transition to an active state -> set "refs" based on previous/old state
	if((oldStateId != STATE_UNDEF) && (mCurrentStateId != STATE_RELAXED) && (mCurrentStateId != STATE_RESTRICTED)) {
		for (Channels::t_access_category accessCategory : mAccessCategories) {	//for each AC
			if(mCurrentState->asDcc.ac[accessCategory].isRef) mCurrentState->asDcc.ac[accessCategory].val = oldState->asDcc.ac[accessCategory].val;
			if(mCurrentState->asTxPower.ac[accessCategory].isRef) mCurrentState->asTxPower.ac[accessCategory].val = oldState->asTxPower.ac[accessCategory].val;
			if(mCurrentState->asPacketInterval.ac[accessCategory].isRef) mCurrentState->asPacketInterval.ac[accessCategory].val = oldState->asPacketInterval.ac[accessCategory].val;
			if(mCurrentState->asDatarate.ac[accessCategory].isRef) mCurrentState->asDatarate.ac[accessCategory].val = oldState->asDatarate.ac[accessCategory].val;
			if(mCurrentState->asCarrierSense.ac[accessCategory].isRef) mCurrentState->asCarrierSense.ac[accessCategory].val = oldState->asCarrierSense.ac[accessCategory].val;
		}
	}

	//reschedule addToken
	mMutexLastTokenAt.lock();
	for (Channels::t_access_category accessCategory : mAccessCategories) {
		rescheduleAddToken(accessCategory);
	}
	mMutexLastTokenAt.unlock();

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

//adds a token=send-permit to the specified bucket of an AC in a fixed time interval
void DCC::addToken(const boost::system::error_code& e, Channels::t_access_category ac) {
	int64_t nowTime = chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1);
	mBucket[ac]->flushQueue(nowTime);												//remove all packets that already expired
	mBucket[ac]->increment();														//add token
	if (ac == Channels::AC_BE)
		cout << "Added token -> available tokens: " << mBucket[ac]->availableTokens << endl;
	sendQueuedPackets(ac);															//send packet(s) from queue with newly added token

	mMutexLastTokenAt.lock();
	mLastTokenAt[ac] = mTimerAddToken[ac]->expires_at();
	mAddedFirstToken[ac] = true;

	mTimerAddToken[ac]->expires_at(mTimerAddToken[ac]->expires_at() + boost::posix_time::milliseconds(currentTokenInterval(ac)*1000.00));	//fixed time interval depending on state and AC
	mTimerAddToken[ac]->async_wait(boost::bind(&DCC::addToken, this, boost::asio::placeholders::error, ac));

	if (ac == Channels::AC_BE)
		cout << "next token at: " << mTimerAddToken[ac]->expires_at() << endl;
	mMutexLastTokenAt.unlock();
}

//sends queued packets from specified LeakyBucket to hardware until out of packets or tokens
void DCC::sendQueuedPackets(Channels::t_access_category ac) {
	while(wrapperPackage::WRAPPER* wrapper = mBucket[ac]->dequeue()) {				//true if packet and token available -> pop 1st packet from queue
		int64_t nowTime = chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1);
		if(wrapper->validuntil() >= nowTime) {										//message still valid
			setMessageLimits(wrapper);

			string byteMessage;
			wrapper->SerializeToString(&byteMessage);
			mSenderToHw->sendToHw(byteMessage);
			cout << "Send wrapper " << wrapper->id() << " to HW" << endl;
			cout << "Remaining tokens: " << mBucket[ac]->availableTokens << endl;
			cout << "Queue length: " << mBucket[ac]->getQueuedPackets() << "\n" << endl;
		}
		else {	//TODO: does this work correctly? flushQueue deletes packets which are ignored here (it seems, packets remain in queue after being sent)		//message expired
			cerr << "--sendQueuedPackets: message expired" << endl;
//			delete wrapper;		//TODO: legal delete?
		}
	}
}

//reschedules timer for next "addToken" when switching to another state
void DCC::rescheduleAddToken(Channels::t_access_category ac) {	//TODO: fix (token interval unchanged but addToken called at very high frequency)
	if(mAddedFirstToken[ac]) {
		//reschedule based on last token that was added (rather than time of state-switch)
		boost::posix_time::ptime newTime = mLastTokenAt[ac] + boost::posix_time::milliseconds(currentTokenInterval(ac)*1000.00);
		cout << "last token at: " << mLastTokenAt[ac] << endl;
		cout << "newTime: " << newTime << endl;

		mTimerAddToken[ac]->expires_at(newTime);
		mTimerAddToken[ac]->async_wait(boost::bind(&DCC::addToken, this, boost::asio::placeholders::error, ac));

		if (ac == Channels::AC_BE) {
			cout << "next token at: " << mTimerAddToken[ac]->expires_at() << endl;
		}
		exit(-1);
	}
}

//sets txPower and data rate for a packet (before being sent)
void DCC::setMessageLimits(wrapperPackage::WRAPPER* wrapper) {
	Channels::t_access_category ac = (Channels::t_access_category) wrapper->priority();		//get AC of packet
	dcc_Mechanism_t dcc_mechanism = currentDcc(ac);

	if(dcc_mechanism == dcc_TPC || dcc_mechanism == dcc_TPC_TRC || dcc_mechanism == dcc_TPC_TDC || dcc_mechanism == dcc_TPC_TRC_TDC || dcc_mechanism == dcc_ALL)
		wrapper->set_txpower(currentTxPower(ac));	//adjust transmission power

	if(dcc_mechanism == dcc_TDC || dcc_mechanism == dcc_TPC_TDC || dcc_mechanism == dcc_TPC_TRC_TDC || dcc_mechanism == dcc_ALL)
		wrapper->set_bitrate(currentDatarate(ac));	//adjust data rate
}

//get parameters of current state
dcc_Mechanism_t DCC::currentDcc(Channels::t_access_category ac) {
	return mCurrentState->asDcc.ac[ac].val;
}
double DCC::currentTxPower(Channels::t_access_category ac) {
	return mCurrentState->asTxPower.ac[ac].val;
}
double DCC::currentTokenInterval(Channels::t_access_category ac) {
	return mCurrentState->asPacketInterval.ac[ac].val;
}
double DCC::currentDatarate(Channels::t_access_category ac) {
	return mCurrentState->asDatarate.ac[ac].val;
}
double DCC::currentCarrierSense(Channels::t_access_category ac) {
	return mCurrentState->asCarrierSense.ac[ac].val;
}


int main() {
	DccConfig config;
	try {
		config.loadParameters("../src/config.xml");
	} catch (exception &e) {
		cerr << "Error while loading config.xml: " << e.what() << endl << flush;
		return EXIT_FAILURE;
	}
	DCC dcc(config);
	dcc.init();

	return EXIT_SUCCESS;
}
