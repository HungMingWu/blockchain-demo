// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <catch2/catch.hpp>
#include <boost/thread/mutex.hpp>
#include "reverselock.h"


TEST_CASE("reverselock_basics")
{
	boost::mutex mutex;
	boost::unique_lock<boost::mutex> lock(mutex);

	REQUIRE(lock.owns_lock());
	{
		reverse_lock<boost::unique_lock<boost::mutex> > rlock(lock);
		REQUIRE(!lock.owns_lock());
	}
	REQUIRE(lock.owns_lock());
}

TEST_CASE("reverselock_errors")
{
	boost::mutex mutex;
	boost::unique_lock<boost::mutex> lock(mutex);

	// Make sure trying to reverse lock an unlocked lock fails
	lock.unlock();

	REQUIRE(!lock.owns_lock());

	bool failed = false;
	try {
		reverse_lock<boost::unique_lock<boost::mutex> > rlock(lock);
	}
	catch (...) {
		failed = true;
	}

	REQUIRE(failed);
	REQUIRE(!lock.owns_lock());

	// Make sure trying to lock a lock after it has been reverse locked fails
	lock.lock();
	REQUIRE(lock.owns_lock());
	{
		reverse_lock<boost::unique_lock<boost::mutex> > rlock(lock);
		REQUIRE(!lock.owns_lock());
	}

	REQUIRE(failed);
	REQUIRE(lock.owns_lock());
}