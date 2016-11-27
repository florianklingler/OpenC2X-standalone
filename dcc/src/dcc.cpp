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
// Florian Klingler <klingler@ccs-labs.org>

#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE

#include "dcc.h"
#include <unistd.h>
#include <string>
#include <iostream>
#include <random>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <signal.h>
#include <common/utility/Utils.h>
#include <common/asn1/CAM.h>
#include <common/asn1/ItsPduHeader.h>
#include <common/asn1/per_decoder.h>

using namespace std;

INITIALIZE_EASYLOGGINGPP

DCC::DCC(DccConfig &config) : mStrand(mIoService) {
	try {
		mGlobalConfig.loadConfigXML("../../common/config/config.xml");
	}
	catch (std::exception &e) {
		cerr << "Error while loading config.xml: " << e.what() << endl;
	}

	string module = "Dcc";
	mConfig = config;

	mMsgUtils = new MessageUtils(module, mGlobalConfig.mExpNo);

	mReceiverFromCa = new CommunicationReceiver(module, "6666", "CAM", mGlobalConfig.mExpNo);
	mReceiverFromDen = new CommunicationReceiver(module, "7777", "DENM", mGlobalConfig.mExpNo);
	mSenderToHw = new SendToHardwareViaMAC(module,mGlobalConfig.mEthernetDevice, mGlobalConfig.mExpNo);
	mReceiverFromHw = new ReceiveFromHardwareViaMAC(module, mGlobalConfig.mExpNo);
	mSenderToServices = new CommunicationSender(module, "5555", mGlobalConfig.mExpNo);
	mSenderToLdm = new CommunicationSender(module, "1234", mGlobalConfig.mExpNo);

	mLogger = new LoggingUtility(module, mGlobalConfig.mExpNo);

	// Use real channel prober when we are not simulating channel load
	if(!mConfig.simulateChannelLoad) {
		mChannelProber = new ChannelProber(mGlobalConfig.mEthernetDevice, mConfig.DCC_measure_interval_Tm, &mIoService, mGlobalConfig.mExpNo); // wlan0
	}

	mPktStatsCollector = new PktStatsCollector(mGlobalConfig.mEthernetDevice, mConfig.DCC_collect_pkt_flush_stats, &mIoService, mGlobalConfig.mExpNo);
	mPktStats = {};		//init stats to 0

	mRandNumberGen = default_random_engine(0);
	mBernoulli = bernoulli_distribution(0);
	mUniform = uniform_real_distribution<double>(-0.1, 0.1);

	mChannelLoad = 0;
	mChannelLoadInTimeUp.reset(mConfig.NDL_timeUp / mConfig.DCC_measure_interval_Tm);		//1
	mChannelLoadInTimeDown.reset(mConfig.NDL_timeDown / mConfig.DCC_measure_interval_Tm);	//5
	initStates(mConfig.NDL_numActiveState);
	mCurrentStateId = STATE_UNDEF;
	setCurrentState(STATE_RELAXED);

	initLeakyBuckets();

	mTimerMeasureChannel = new boost::asio::deadline_timer(mIoService, boost::posix_time::millisec(mConfig.DCC_measure_interval_Tm*1000));	//1000
	mTimerMeasurePktStats = new boost::asio::deadline_timer(mIoService, boost::posix_time::millisec(mConfig.DCC_collect_pkt_flush_stats*1000));	//1000
	mTimerStateUpdate = new boost::asio::deadline_timer(mIoService, boost::posix_time::millisec(mConfig.NDL_minDccSampling*1000));	//1000
	mTimerDccInfo = new boost::asio::deadline_timer(mIoService, boost::posix_time::millisec(mConfig.dccInfoInterval));

	for (Channels::t_access_category accessCategory : mAccessCategories) {	//create and map the timers for the four ACs
		mTimerAddToken.insert(make_pair(accessCategory, new boost::asio::deadline_timer(mIoService, boost::posix_time::millisec(currentTokenInterval(accessCategory)*1000.00))));
		mAddedFirstToken[accessCategory] = false;
	}
	mStatIdx = 0;
	mLogger->logStats("Index \tChannel Load \tFlush req in BE \tFlush not req in BE \tFlush req in BE in last 1 sec \tFlush not req in BE in last 1 sec");
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
	delete mSenderToLdm;

	//stop and delete timers
	mTimerMeasureChannel->cancel();
	mTimerStateUpdate->cancel();
	mTimerDccInfo->cancel();
	delete mTimerMeasureChannel;
	delete mTimerStateUpdate;
	delete mTimerDccInfo;

	for (Channels::t_access_category accessCategory : mAccessCategories) {	//for each AC
		mTimerAddToken[accessCategory]->cancel();
		delete mTimerAddToken[accessCategory];	//delete deadline_timer
		delete mBucket[accessCategory];			//delete LeakyBucket
	}

	// delete channel prober
	if(!mConfig.simulateChannelLoad) {
		delete mChannelProber;
	}

	delete mMsgUtils;

	delete mLogger;
}

void DCC::init() {
	// Initialize channel prober only when we are not simulating
	if(!mConfig.simulateChannelLoad) {
		mChannelProber->init();
	}

	// Initialize module for collecting statistics related to flushing outdated packets in hardware queues
	mPktStatsCollector->init();

	//create and start threads
	mThreadReceiveFromCa = new boost::thread(&DCC::receiveFromCa2, this);
	mThreadReceiveFromDen = new boost::thread(&DCC::receiveFromDen, this);
	mThreadReceiveFromHw = new boost::thread(&DCC::receiveFromHw2, this);

	//start timers
	mTimerMeasureChannel->async_wait(mStrand.wrap(boost::bind(&DCC::measureChannel, this, boost::asio::placeholders::error)));
	mTimerMeasurePktStats->async_wait(mStrand.wrap(boost::bind(&DCC::measurePktStats, this, boost::asio::placeholders::error)));
	mTimerStateUpdate->async_wait(mStrand.wrap(boost::bind(&DCC::updateState, this, boost::asio::placeholders::error)));
	mTimerDccInfo->async_wait(mStrand.wrap(boost::bind(&DCC::sendDccInfo, this, boost::asio::placeholders::error)));

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
	mBucket.insert(make_pair(Channels::AC_VI, new LeakyBucket<dataPackage::DATA>(mConfig.bucketSize_AC_VI, mConfig.queueSize_AC_VI, mGlobalConfig.mExpNo)));
	mBucket.insert(make_pair(Channels::AC_VO, new LeakyBucket<dataPackage::DATA>(mConfig.bucketSize_AC_VO, mConfig.queueSize_AC_VO, mGlobalConfig.mExpNo)));
	mBucket.insert(make_pair(Channels::AC_BE, new LeakyBucket<dataPackage::DATA>(mConfig.bucketSize_AC_BE, mConfig.queueSize_AC_BE, mGlobalConfig.mExpNo)));
	mBucket.insert(make_pair(Channels::AC_BK, new LeakyBucket<dataPackage::DATA>(mConfig.bucketSize_AC_BK, mConfig.queueSize_AC_BK, mGlobalConfig.mExpNo)));
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

		Channels::t_access_category ac = (Channels::t_access_category) data->priority();
		int64_t nowTime = Utils::currentTime();
		mBucket[ac]->flushQueue(nowTime);

		mLogger->logInfo("");						//for readability
		bool enqueued = mBucket[ac]->enqueue(data, data->validuntil());
		if (enqueued) {
			mLogger->logInfo("AC "+ to_string(ac) + ": received and enqueued CAM " + to_string(data->id()) + ", queue length: " + to_string(mBucket[ac]->getQueuedPackets()));
			sendQueuedPackets(ac);
		}
		else {
			mLogger->logInfo("AC "+ to_string(ac) + ": received and dropped CAM " + to_string(data->id()) + ", queue full -> length: " + to_string(mBucket[ac]->getQueuedPackets()));
		}
	}
}

void DCC::receiveFromCa2() {
	string encodedData;					//serialized DATA
	dataPackage::DATA* data;			//deserialized DATA

	while (1) {
		pair<string, string> received = mReceiverFromCa->receive();
		encodedData = received.second;

		data = new dataPackage::DATA();
		data->ParseFromString(encodedData);		//deserialize DATA

		string encodedCam = data->content();

		Channels::t_access_category ac = (Channels::t_access_category) data->priority();
		int64_t nowTime = Utils::currentTime();
		mBucket[ac]->flushQueue(nowTime);

		mLogger->logInfo("");						//for readability
		bool enqueued = mBucket[ac]->enqueue(data, data->validuntil());
		if (enqueued) {
			mLogger->logInfo("AC "+ to_string(ac) + ": received and enqueued CAM " + to_string(data->id()) + ", queue length: " + to_string(mBucket[ac]->getQueuedPackets()));
			sendQueuedPackets(ac);
		}
		else {
			mLogger->logInfo("AC "+ to_string(ac) + ": received and dropped CAM " + to_string(data->id()) + ", queue full -> length: " + to_string(mBucket[ac]->getQueuedPackets()));
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

		Channels::t_access_category ac = (Channels::t_access_category) data->priority();
		int64_t nowTime = Utils::currentTime();
		mBucket[ac]->flushQueue(nowTime);

		mLogger->logInfo("");						//for readability
		bool enqueued = mBucket[ac]->enqueue(data, data->validuntil());
		if (enqueued) {
			mLogger->logInfo("AC "+ to_string(ac) + ": received and enqueued DENM " + to_string(data->id()) + ", queue length: " + to_string(mBucket[ac]->getQueuedPackets()));
			sendQueuedPackets(ac);
		}
		else {
			mLogger->logInfo("AC "+ to_string(ac) + ": received and dropped DENM " + to_string(data->id()) + ", queue full -> length: " + to_string(mBucket[ac]->getQueuedPackets()));
		}
	}
}

void DCC::receiveFromHw() {
	pair<string,string> receivedData;		//MAC Sender, serialized DATA
	string* senderMac = &receivedData.first;
	string* serializedData = &receivedData.second;
	dataPackage::DATA data;
	mLogger->logInfo("start receiving via Hardware");
	while (1) {
		receivedData = mReceiverFromHw->receive();	//receive serialized DATA


		//check whether the mac of the sender and our own mac are the same and discard the package if we want to ignore those packages
		if(mConfig.ignoreOwnMessages && senderMac->compare(mSenderToHw->mOwnMac) == 0){
			mLogger->logDebug("received own Message, discarding");
			continue;
		}

		data.ParseFromString(*serializedData);		//deserialize DATA
		//processing...
		mLogger->logInfo("forward message from "+*senderMac +" from HW to services");
		switch(data.type()) {								//send serialized DATA to corresponding module
			case dataPackage::DATA_Type_CAM: 		mSenderToServices->send("CAM", *serializedData);	break;
			case dataPackage::DATA_Type_DENM:		mSenderToServices->send("DENM", *serializedData);	break;
			default:	break;
		}
	}
}


void DCC::receiveFromHw2() {
	pair<ReceivedPacketInfo, string> receivedData;		//MAC Sender, serialized DATA
	ReceivedPacketInfo* pktInfo = &receivedData.first;
	string* serializedData = &receivedData.second;
	mLogger->logInfo("start receiving via Hardware");
	while (1) {
		receivedData = mReceiverFromHw->receiveWithGeoNetHeader();	//receive serialized DATA


		//check whether the mac of the sender and our own mac are the same and discard the package if we want to ignore those packages
		if(mConfig.ignoreOwnMessages && pktInfo->mSenderMac.compare(mSenderToHw->mOwnMac) == 0) {
			mLogger->logDebug("received own Message, discarding");
			continue;
		}

		mLogger->logInfo("forward packet from "+ pktInfo->mSenderMac +" from HW to services");
		switch(pktInfo->mType) {								//send serialized DATA to corresponding module
			case dataPackage::DATA_Type_CAM:
				mSenderToServices->send("CAM", *serializedData);
				break;
			case dataPackage::DATA_Type_DENM:
				mSenderToServices->send("DENM", *serializedData);
				break;
			default:
				break;
		}
	}
}

//periodically sends network info to LDM in form of four protobuf packets (one for each AC)
void DCC::sendDccInfo(const boost::system::error_code& ec) {
	infoPackage::DccInfo dccInfo;
	string serializedDccInfo;

	for (Channels::t_access_category ac : mAccessCategories) {	//send for each AC
		dccInfo.set_time(Utils::currentTime());
		dccInfo.set_channelload(mChannelLoad);

		//set state
		if (mCurrentStateId == STATE_RELAXED) {
			dccInfo.set_state("relaxed");
		}
		else if (mCurrentStateId == STATE_ACTIVE1) {
			dccInfo.set_state("active");
		}
		else if (mCurrentStateId == STATE_RESTRICTED) {
			dccInfo.set_state("restricted");
		}

		//set access category and flush stats
		switch (ac) {
		case Channels::AC_VI:
			dccInfo.set_accesscategory("VI");
			dccInfo.set_flushreqpackets(mPktStats.vi_flush_req);
			dccInfo.set_flushnotreqpackets(mPktStats.vi_flush_not_req);
			break;
		case Channels::AC_VO:
			dccInfo.set_accesscategory("VO");
			dccInfo.set_flushreqpackets(mPktStats.vo_flush_req);
			dccInfo.set_flushnotreqpackets(mPktStats.vo_flush_not_req);
			break;
		case Channels::AC_BE:
			dccInfo.set_accesscategory("BE");
			dccInfo.set_flushreqpackets(mPktStats.be_flush_req);
			dccInfo.set_flushnotreqpackets(mPktStats.be_flush_not_req);
			break;
		case Channels::AC_BK:
			dccInfo.set_accesscategory("BK");
			dccInfo.set_flushreqpackets(mPktStats.bk_flush_req);
			dccInfo.set_flushnotreqpackets(mPktStats.bk_flush_not_req);
			break;
		case Channels::NO_AC:
			dccInfo.set_accesscategory("NO");
			break;
		}

		dccInfo.set_availabletokens(mBucket[ac]->getAvailableTokens());
		dccInfo.set_queuedpackets(mBucket[ac]->getQueuedPackets());
		dccInfo.set_dccmechanism(currentDcc(ac));
		dccInfo.set_txpower(currentTxPower(ac));
		dccInfo.set_tokeninterval(currentTokenInterval(ac));
		dccInfo.set_datarate(currentDatarate(ac));
		dccInfo.set_carriersense(currentCarrierSense(ac));

		dccInfo.SerializeToString(&serializedDccInfo);
		mSenderToLdm->send("dccInfo", serializedDccInfo);
	}

	mTimerDccInfo->expires_at(mTimerDccInfo->expires_at() + boost::posix_time::milliseconds(mConfig.dccInfoInterval));	//1s
	mTimerDccInfo->async_wait(mStrand.wrap(boost::bind(&DCC::sendDccInfo, this, boost::asio::placeholders::error)));
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

	if(mConfig.simulateChannelLoad) {
		mChannelLoad = simulateChannelLoad();
	} else {
		mChannelLoad = mChannelProber->getChannelLoad();
	}

	mChannelLoadInTimeUp.insert(mChannelLoad);	//add to RingBuffer
	mChannelLoadInTimeDown.insert(mChannelLoad);

	mTimerMeasureChannel->expires_at(mTimerMeasureChannel->expires_at() + boost::posix_time::milliseconds(mConfig.DCC_measure_interval_Tm*1000));	//1s
	mTimerMeasureChannel->async_wait(mStrand.wrap(boost::bind(&DCC::measureChannel, this, boost::asio::placeholders::error)));
}

//periodically gets and sets the packet stats (about flushed packets)
void DCC::measurePktStats(const boost::system::error_code& ec) {
	if (ec == boost::asio::error::operation_aborted) {	// Timer was cancelled, do not measure channel
		return;
	}

	mPktStats = mPktStatsCollector->getPktStats();
	mLogger->logStats(to_string(mStatIdx++)
			+ "\t" + to_string(mChannelProber->getChannelLoad())
			+ "\t" + to_string(mPktStats.be_flush_req) + "\t" + to_string(mPktStats.be_flush_not_req)
			+ "\t" + to_string(mPktStats.be_flush_req - prev_be_flush_req) + "\t" + to_string(mPktStats.be_flush_not_req - prev_be_flush_not_req)
		);
	prev_be_flush_req = mPktStats.be_flush_req;
	prev_be_flush_not_req = mPktStats.be_flush_not_req;

	mTimerMeasurePktStats->expires_at(mTimerMeasurePktStats->expires_at() + boost::posix_time::milliseconds(mConfig.DCC_collect_pkt_flush_stats*1000));	//1s
	mTimerMeasurePktStats->async_wait(mStrand.wrap(boost::bind(&DCC::measurePktStats, this, boost::asio::placeholders::error)));
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
		mLogger->logInfo("Current state: Undefined");
		break;
	case STATE_RELAXED:
		mLogger->logInfo("Current state: Relaxed");
		break;
	case STATE_ACTIVE1:
		mLogger->logInfo("Current state: Active1");
		break;
	case STATE_RESTRICTED:
		mLogger->logInfo("Current state: Restricted");
		break;
	default: break;
	}
}

//adds a token=send-permit to the specified bucket of an AC in a fixed time interval
void DCC::addToken(const boost::system::error_code& ec, Channels::t_access_category ac) {
	if (ec == boost::asio::error::operation_aborted) {	// Timer was cancelled, do not add token
		return;
	}

	int64_t nowTime = Utils::currentTime();
	mBucket[ac]->flushQueue(nowTime);												//remove all packets that already expired
	mBucket[ac]->increment();														//add token
	mLogger->logDebug("AC " + to_string(ac) + ": added token -> available tokens: " + to_string(mBucket[ac]->availableTokens));
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
		int64_t nowTime = Utils::currentTime();
		if(data->validuntil() >= nowTime) {							//message still valid
			setMessageLimits(data);

			string byteMessage;
			byteMessage = data->content();
			mSenderToHw->sendWithGeoNet(&byteMessage, ac, data->type());
			mLogger->logInfo("AC " + to_string(ac) + ": Sent data " + to_string(data->id()) + " to HW -> queue length: " + to_string(mBucket[ac]->getQueuedPackets()) + ", tokens: " + to_string(mBucket[ac]->availableTokens));
			delete data;
		}
		else {														//message expired
			mBucket[ac]->increment();								//undo decrement in dequeue because message expired and token is still valid (nothing was sent)
			mLogger->logInfo("message (packet " + to_string(data->id()) + ") expired");
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
		config.loadParameters("../config/config.xml");
	} catch (exception &e) {
		cerr << "Error while loading config.xml: " << e.what() << endl << flush;
		return EXIT_FAILURE;
	}
	DCC dcc(config);
	dcc.init();

	return EXIT_SUCCESS;
}
