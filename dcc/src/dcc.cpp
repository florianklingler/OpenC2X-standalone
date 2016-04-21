
#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include "dcc.h"
#include <unistd.h>
#include <string>
#include <iostream>
#include <random>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <signal.h>

using namespace std;

INITIALIZE_EASYLOGGINGPP

DCC::DCC(DccConfig &config) : mStrand(mIoService) {
	string module = "Dcc";
	mConfig = config;
	mReceiverFromCa = new CommunicationReceiver(module, "6666", "CAM");
	mReceiverFromDen = new CommunicationReceiver(module, "7777", "DENM");
	mSenderToHw = new SendToHardwareViaMAC();
	mReceiverFromHw = new ReceiveFromHardwareViaMAC(module);
	mSenderToServices = new CommunicationSender(module, "5555");

	// Use real channel prober when we are not simulating channel load
	if(!mConfig.simulateChannelLoad) {
		mChannelProber = new ChannelProber("wlan1-ath9k"); // wlan0
	}

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
	//stop and delete threads
	mThreadReceiveFromCa->join();
	mThreadReceiveFromDen->join();
	mThreadReceiveFromHw->join();
	delete mThreadReceiveFromCa;
	delete mThreadReceiveFromDen;
	delete mThreadReceiveFromHw;

	//delete sender and receiver
	delete mReceiverFromCa;
	delete mReceiverFromDen;
	delete mReceiverFromHw;
	delete mSenderToHw;
	delete mSenderToServices;

	//stop and delete timers
	mTimerMeasure->cancel();
	mTimerStateUpdate->cancel();
	delete mTimerMeasure;
	delete mTimerStateUpdate;

	for (Channels::t_access_category accessCategory : mAccessCategories) {	//for each AC
		mTimerAddToken[accessCategory]->cancel();
		delete mTimerAddToken[accessCategory];	//delete deadline_timer
		delete mBucket[accessCategory];			//delete LeakyBucket
	}

	// delete channel prober
	if(!mConfig.simulateChannelLoad) {
		delete mChannelProber;
	}
}

void DCC::init() {
	// Initialize channel prober only when we are not simulating
	if(!mConfig.simulateChannelLoad) {
		mChannelProber->init();
	}
	
	//create and start threads
	mThreadReceiveFromCa = new boost::thread(&DCC::receiveFromCa, this);
	mThreadReceiveFromDen = new boost::thread(&DCC::receiveFromDen, this);
	mThreadReceiveFromHw = new boost::thread(&DCC::receiveFromHw, this);

	//start timers
	mTimerMeasure->async_wait(mStrand.wrap(boost::bind(&DCC::measureChannel, this, boost::asio::placeholders::error)));
	mTimerStateUpdate->async_wait(mStrand.wrap(boost::bind(&DCC::updateState, this, boost::asio::placeholders::error)));

	for (Channels::t_access_category accessCategory : mAccessCategories) {	//for each AC
		mTimerAddToken[accessCategory]->async_wait(mStrand.wrap(boost::bind(&DCC::addToken, this, boost::asio::placeholders::error, accessCategory)));	//start timer
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

//initializes leaky buckets
void DCC::initLeakyBuckets() {
	mBucket.insert(make_pair(Channels::AC_VI, new LeakyBucket<dataPackage::DATA>(mConfig.bucketSize_AC_VI, mConfig.queueSize_AC_VI)));
	mBucket.insert(make_pair(Channels::AC_VO, new LeakyBucket<dataPackage::DATA>(mConfig.bucketSize_AC_VO, mConfig.queueSize_AC_VO)));
	mBucket.insert(make_pair(Channels::AC_BE, new LeakyBucket<dataPackage::DATA>(mConfig.bucketSize_AC_BE, mConfig.queueSize_AC_BE)));
	mBucket.insert(make_pair(Channels::AC_BK, new LeakyBucket<dataPackage::DATA>(mConfig.bucketSize_AC_BK, mConfig.queueSize_AC_BK)));
}


//////////////////////////////////////////
//Send & receive

void DCC::receiveFromCa() {
	string serializedData;					//serialized DATA
	dataPackage::DATA* data;			//deserialized DATA

	while (1) {
		pair<string, string> received = mReceiverFromCa->receive();
		serializedData = received.second;

		data = new dataPackage::DATA();
		data->ParseFromString(serializedData);		//deserialize DATA
		cout << "received new CAM (packet " << data->id() << ") -> enqueue to BE" << endl;

		Channels::t_access_category ac = (Channels::t_access_category) data->priority();
		int64_t nowTime = chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1);
		mBucket[ac]->flushQueue(nowTime);
		bool enqueued = mBucket[ac]->enqueue(data, data->validuntil());
		if (enqueued) {
			cout << "Queue length: " << mBucket[ac]->getQueuedPackets() << endl;
			sendQueuedPackets(ac);
		}
	}
}

void DCC::receiveFromDen() {
	string serializedData;		//serialized DATA
	dataPackage::DATA* data;	//deserialized DATA

	while (1) {
		pair<string, string> received = mReceiverFromDen->receive();
		serializedData = received.second;

		data = new dataPackage::DATA();
		data->ParseFromString(serializedData);		//deserialize DATA
		cout << "received new DENM (packet " << data->id() << ") -> enqueue to BE" << endl;

		Channels::t_access_category ac = (Channels::t_access_category) data->priority();
		int64_t nowTime = chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1);
		mBucket[ac]->flushQueue(nowTime);
		bool enqueued = mBucket[ac]->enqueue(data, data->validuntil());
		if (enqueued) {
			cout << "Queue length: " << mBucket[ac]->getQueuedPackets() << endl;
			sendQueuedPackets(ac);
		}
	}
}

void DCC::receiveFromHw() {
	pair<string,string> receivedData;		//MAC Sender, serialized DATA
	string* serializedData = &receivedData.second;
	dataPackage::DATA data;

	while (1) {
		receivedData = mReceiverFromHw->receive();	//receive serialized DATA
		data.ParseFromString(*serializedData);	//deserialize DATA

		//processing...
		cout << "forward message from HW to services" << endl;
		switch(data.type()) {								//send serialized DATA to corresponding module
			case dataPackage::DATA_Type_CAM: 		mSenderToServices->send("CAM", *serializedData);	break;
			case dataPackage::DATA_Type_DENM:		mSenderToServices->send("DENM", *serializedData);	break;
			default:	break;
		}
	}
}

//////////////////////////////////////////
//Decentralized congestion control

//simulates realistic channel load; to be replaced with actual measurements
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
void DCC::measureChannel(const boost::system::error_code& ec) {
	if (ec == boost::asio::error::operation_aborted) {	// Timer was cancelled, do not measure channel
		return;
	}

	double channelLoad = 0;
	if(mConfig.simulateChannelLoad) {
		channelLoad = simulateChannelLoad();
	} else {
		channelLoad = mChannelProber->getChannelLoad();
	}

	mChannelLoadInTimeUp.insert(channelLoad);	//add to RingBuffer
	mChannelLoadInTimeDown.insert(channelLoad);

	mTimerMeasure->expires_at(mTimerMeasure->expires_at() + boost::posix_time::milliseconds(mConfig.DCC_measure_interval_Tm*1000));	//1s
	mTimerMeasure->async_wait(mStrand.wrap(boost::bind(&DCC::measureChannel, this, boost::asio::placeholders::error)));
}

//updates state according to recent channel load measurements
void DCC::updateState(const boost::system::error_code& ec) {	//TODO: adjust to more than 1 active state
	if (ec == boost::asio::error::operation_aborted) {	// Timer was cancelled, do not update state
		return;
	}

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
	mTimerStateUpdate->async_wait(mStrand.wrap(boost::bind(&DCC::updateState, this, boost::asio::placeholders::error)));
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
	for (Channels::t_access_category accessCategory : mAccessCategories) {
		rescheduleAddToken(accessCategory);
	}

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
void DCC::addToken(const boost::system::error_code& ec, Channels::t_access_category ac) {
	if (ec == boost::asio::error::operation_aborted) {	// Timer was cancelled, do not add token
		return;
	}

	int64_t nowTime = chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1);
	mBucket[ac]->flushQueue(nowTime);												//remove all packets that already expired
	mBucket[ac]->increment();														//add token
	if (ac == Channels::AC_BE) {
		cout << "Added token -> available tokens: " << mBucket[ac]->availableTokens << endl;
	}
	sendQueuedPackets(ac);															//send packet(s) from queue with newly added token

	mMutexLastTokenAt.lock();
	mLastTokenAt[ac] = mTimerAddToken[ac]->expires_at();
	mAddedFirstToken[ac] = true;

	mTimerAddToken[ac]->expires_at(mLastTokenAt[ac] + boost::posix_time::milliseconds(currentTokenInterval(ac)*1000.00));	//fixed time interval depending on state and AC
	mTimerAddToken[ac]->async_wait(mStrand.wrap(boost::bind(&DCC::addToken, this, boost::asio::placeholders::error, ac)));

	mMutexLastTokenAt.unlock();
}

//sends queued packets from specified LeakyBucket to hardware until out of packets or tokens
void DCC::sendQueuedPackets(Channels::t_access_category ac) {
	while(dataPackage::DATA* data = mBucket[ac]->dequeue()) { 		//true if packet and token available -> pop 1st packet from queue and remove token
		int64_t nowTime = chrono::high_resolution_clock::now().time_since_epoch() / chrono::nanoseconds(1);
		if(data->validuntil() >= nowTime) {							//message still valid
			setMessageLimits(data);

			string byteMessage;
			data->SerializeToString(&byteMessage);
			mSenderToHw->send(&byteMessage,0);
			cout << "Send data (packet " << data->id() << ") to HW" << endl;
			cout << "Remaining tokens: " << mBucket[ac]->availableTokens << endl;
			cout << "Queue length: " << mBucket[ac]->getQueuedPackets() << "\n" << endl;
			delete data;
		}
		else {														//message expired
			mBucket[ac]->increment();								//undo decrement in dequeue because message expired and token is still valid (nothing was sent)
			cerr << "--sendQueuedPackets: message (packet " << data->id() << ") expired" << endl;
			delete data;
		}
	}
}

//reschedules timer for next "addToken" when switching to another state
void DCC::rescheduleAddToken(Channels::t_access_category ac) {
	if(mAddedFirstToken[ac]) {
		mMutexLastTokenAt.lock();
		//reschedule based on last token that was added (rather than time of state-switch)
		boost::posix_time::ptime newTime = mLastTokenAt[ac] + boost::posix_time::milliseconds(currentTokenInterval(ac)*1000.00);

		mTimerAddToken[ac]->expires_at(newTime);
		mTimerAddToken[ac]->async_wait(mStrand.wrap(boost::bind(&DCC::addToken, this, boost::asio::placeholders::error, ac)));

		mMutexLastTokenAt.unlock();
	}
}

//sets txPower and data rate for a packet (before being sent)
void DCC::setMessageLimits(dataPackage::DATA* data) {
	Channels::t_access_category ac = (Channels::t_access_category) data->priority();		//get AC of packet
	dcc_Mechanism_t dcc_mechanism = currentDcc(ac);

	if(dcc_mechanism == dcc_TPC || dcc_mechanism == dcc_TPC_TRC || dcc_mechanism == dcc_TPC_TDC || dcc_mechanism == dcc_TPC_TRC_TDC || dcc_mechanism == dcc_ALL)
		data->set_txpower(currentTxPower(ac));	//adjust transmission power

	if(dcc_mechanism == dcc_TDC || dcc_mechanism == dcc_TPC_TDC || dcc_mechanism == dcc_TPC_TRC_TDC || dcc_mechanism == dcc_ALL)
		data->set_bitrate(currentDatarate(ac));	//adjust data rate
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

void onSigTermOk(int sig) {
	cout << "Signal " << sig << " received. Requesting exit." << endl;
	exit(0);
}

int main() {
	signal(SIGINT, &onSigTermOk);
	signal(SIGQUIT, &onSigTermOk);
	signal(SIGABRT, &onSigTermOk);
	signal(SIGKILL, &onSigTermOk);
	signal(SIGTERM, &onSigTermOk);

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
