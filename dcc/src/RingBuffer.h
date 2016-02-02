#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

#include <assert.h>
#include <list>
#include <cstdlib>
#include <algorithm>

using namespace std;

template<typename T> class RingBuffer {
public:
	size_t maxSize;
	list<T> entries;
	typename list<T>::iterator nextInsert;

	RingBuffer(size_t maxSize = 0) :
		maxSize(0) {
		reset(maxSize);
	}

	~RingBuffer() {
	}

	void reset(size_t maxSize) {
		entries.clear();
		this->maxSize = maxSize;
		nextInsert = entries.end();
	}

	void insert(const T& o) {
		assert(maxSize > 0);
		if (nextInsert == entries.end()) {
			if (entries.size() < maxSize) {
				// special case: RingBuffer not yet full
				entries.push_back(o);
				return;
			}
			else {
				nextInsert = entries.begin();
			}
		}
		*nextInsert = o;
		nextInsert++;
	}

	T min() {
		return *min_element(entries.begin(), entries.end());
	}

	T max() {
		return *max_element(entries.begin(), entries.end());
	}

	double avg() {
		double sum = accumulate(entries.begin(), entries.end(), 0.0);
		return (double) (sum / (double) entries.size());
	}
};


#endif /* RINGBUFFER_H_ */
