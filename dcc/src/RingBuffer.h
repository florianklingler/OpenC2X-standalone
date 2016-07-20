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


#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

/**
 * @addtogroup dcc
 * @{
 */

#include <assert.h>
#include <list>
#include <cstdlib>
#include <algorithm>

/**
 * Template class to hold channel load measurements (or anything else) in a ring buffer.
 */
template<typename T> class RingBuffer {
public:
	size_t maxSize;
	std::list<T> entries;
	typename std::list<T>::iterator nextInsert;

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

/**
 * @}
 */

#endif /* RINGBUFFER_H_ */
