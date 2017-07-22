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

#ifndef LEAKYBUCKET_H_
#define LEAKYBUCKET_H_

/**
 * @addtogroup dcc
 * @{
 */

#include <cstdlib>
#include <list>
#include <mutex>
#include <iostream>
#include <common/utility/LoggingUtility.h>


/**
 * LeakyBucket consists of a bucket for tokens (= send-permits) and a queue
 * for packets that need to be sent.
 */
template<typename T> class LeakyBucket {
private:
	LoggingUtility* mLogger;
public:
	/**
	 * Max. number of tokens
	 */
	size_t maxBucketSize;		//max. number of tokens
	/**
	 * Available number of tokens = send-permits.
	 */
	size_t availableTokens;		//available number of tokens = send-permits
	std::mutex mutex_bucket;	//mutex to control access
	std::mutex mutex_queue;
	typename std::list<std::pair<int64_t, T*> > queue;	//pair: validUntil, packet
	size_t queueSize;

	LeakyBucket(size_t bucketSize, size_t queueSize, int expNo, std::string loggingConf, std::string statisticConf) {
		this->maxBucketSize = bucketSize;
		availableTokens = 0;
		this->queueSize = queueSize;
		mLogger = new LoggingUtility("Dcc", expNo, loggingConf, statisticConf);
	}

	~LeakyBucket() {
		typename std::list<std::pair<int64_t, T*> >::iterator it = queue.begin();
		while(it!=queue.end()) {
			typename std::pair<int64_t, T*> p = *it;
			T* msg = p.second;
			delete msg;
			queue.erase(it++);
		}
		delete mLogger;
	}

	/** Getter for number of available tokens.
	 *
	 * @return Number of available tokens
	 */
	int getAvailableTokens() {
		mutex_bucket.lock();
		int numTokens = availableTokens;
		mutex_bucket.unlock();
		return numTokens;
	}

	/**
	 * Getter for number of queued packets.
	 * @return Number of queued packets
	 */
	int getQueuedPackets() {
		mutex_queue.lock();
		int numQueuedPackets = queue.size();
		mutex_queue.unlock();
		return numQueuedPackets;
	}

	/**
	 *
	 * @return true if and only if the queue is empty
	 */
	bool isQueueEmpty() {
		mutex_queue.lock();
		bool ret = queue.empty();
		mutex_queue.unlock();
		return ret;
	}

	/**
	 * Adds a token to the bucket if not full.
	 * @return true if the token was added successfully
	 */
	bool increment() {
		mutex_bucket.lock();
		bool ret = false;
		if(availableTokens < maxBucketSize) {
			availableTokens++;
			ret = true;
		} else {
			ret = false;
		}
		mutex_bucket.unlock();
		return ret;
	}

	/**
	 * Adds a packet to the queue if not full.
	 * @param p Packet to be enqueued
	 * @param validUntil Life time of the packet
	 * @return true if the packet was enqueued successfully
	 */
	bool enqueue(T* p, int64_t validUntil) {
		mutex_queue.lock();
		bool ret = false;
		if(queue.size() < queueSize) {
			queue.push_back(std::make_pair(validUntil, p));
			ret = true;
		} else {
			ret = false;
		}
		mutex_queue.unlock();
		return ret;
	}

	/**
	 * Removes a token from the bucket if there is any.
	 * @return true if a token was removed successfully
	 */
	bool decrement() {
		mutex_bucket.lock();
		bool ret = false;
		if(availableTokens > 0) {
			availableTokens--;
			ret = true;
		} else {
			ret = false;
		}
		mutex_bucket.unlock();
		return ret;
	}

	/**
	 * Removes and returns the first packet from the queue if there is any and there is a token available.
	 * @return The first packet (or NULL if no packet or token available)
	 */
	T* dequeue() {
		mutex_queue.lock();
		T* ret = NULL;
		if(queue.size() > 0) {
			if(decrement()) {
				ret = queue.front().second;
				queue.pop_front();
			} else {
				ret = NULL;
			}
		} else {
			ret = NULL;
		}
		mutex_queue.unlock();
		return ret;
	}

	/**
	 * Removes all packets that expire before a given time t.
	 * @param t Expiration time
	 * @param eraseAll If set to true, all packets are erased, regardless of their validity
	 */
	void flushQueue(int64_t t, bool eraseAll=false) {
		mutex_queue.lock();
		typename std::list<std::pair<int64_t, T*> >::iterator it = queue.begin();
		while(it!=queue.end()) {
			typename std::pair<int64_t, T*> p = *it;
			int64_t validUntil = p.first;
			T* msg = p.second;

			if(validUntil < t || eraseAll) {
				mLogger->logInfo("flushQueue: message " + std::to_string(msg->id()) + " expired");
				delete msg;
				queue.erase(it++);
			} else {
				it++;
			}
		}
		mutex_queue.unlock();
	}

	/**
	 * Prints the whole queue (for debugging).
	 */
	void printQueue() {
		mutex_queue.lock();
		std::cout << "printing queue" << std::endl;

		typename std::list<std::pair<int64_t, T*>>::iterator it;
		for(it=queue.begin(); it != queue.end(); it++) {
			std::cout << "packet " << it->second->id() << ", createTime: " << it->second->createtime() << ", validUntil: " << it->first << std::endl;
		}
		mutex_queue.unlock();
	}
};

/**
 * @}
 */

#endif /* LEAKYBUCKET_H_ */
