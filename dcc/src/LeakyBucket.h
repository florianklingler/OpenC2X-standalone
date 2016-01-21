#ifndef LEAKYBUCKET_H_
#define LEAKYBUCKET_H_

#include <cstdlib>
#include <list>
#include <mutex>
#include <iostream>


//LeakyBucket consists of a bucket for tokens, i.e. send-permits, and a queue for packets that need to be sent
template<typename T> class LeakyBucket {
public:
	size_t maxBucketSize;		//max. number of tokens
	size_t availableTokens;		//available number of tokens = send-permits
	std::mutex mutex_bucket;	//mutex to control access
	std::mutex mutex_queue;
	typename std::list<std::pair<int64_t, T*> > queue;	//pair: validUntil, packet
	size_t queueSize;

	LeakyBucket(size_t bucketSize, size_t queueSize) {
		this->maxBucketSize = bucketSize;
		availableTokens = 0;
		this->queueSize = queueSize;
	}

	~LeakyBucket() {
		typename std::list<std::pair<int64_t, T*> >::iterator it = queue.begin();
		while(it!=queue.end()) {
			typename std::pair<int64_t, T*> p = *it;
			T* msg = p.second;
			delete msg;
			queue.erase(it++);
		}
	}

	//number of available tokens
	int getAvailableTokens() {
		mutex_bucket.lock();
		int numTokens = availableTokens;
		mutex_bucket.unlock();
		return numTokens;
	}

	//number of queued packets
	int getQueuedPackets() {
		mutex_queue.lock();
		int numQueuedPackets = queue.size();
		mutex_queue.unlock();
		return numQueuedPackets;
	}

	//returns true iff queue is empty
	bool isQueueEmpty() {
		mutex_queue.lock();
		bool ret = queue.empty();
		mutex_queue.unlock();
		return ret;
	}

	//adds a token to the bucket; returns false iff bucket is full
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

	//adds packet to the queue; returns false if queue is full
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

	//removes token from the bucket; returns false if bucket is empty
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

	//removes and returns first packet from the queue; if queue is empty or no tokens are available, NULL is returned
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

	//removes all packets that expire before a given time t (or removes all packets if eraseAll=true)
	void flushQueue(int64_t t, bool eraseAll=false) {
		mutex_queue.lock();
		typename std::list<std::pair<int64_t, T*> >::iterator it = queue.begin();
		while(it!=queue.end()) {
			typename std::pair<int64_t, T*> p = *it;
			int64_t validUntil = p.first;
			T* msg = p.second;

			if(validUntil < t || eraseAll) {
				std::cout << "--flushQueue: message " << msg->id() << " expired" << std::endl;
				//delete msg;	//TODO
				queue.erase(it++);
			} else {
				it++;
			}
		}
		mutex_queue.unlock();
	}
};


#endif /* LEAKYBUCKET_H_ */
