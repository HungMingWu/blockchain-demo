// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//

#include <catch2/catch.hpp>
#include "timedata.h"


TEST_CASE("util_MedianFilter")
{
	CMedianFilter<int> filter(5, 15);

	REQUIRE(filter.median() == 15);

	filter.input(20); // [15 20]
	REQUIRE(filter.median() == 17);

	filter.input(30); // [15 20 30]
	REQUIRE(filter.median() == 20);

	filter.input(3); // [3 15 20 30]
	REQUIRE(filter.median() == 17);

	filter.input(7); // [3 7 15 20 30]
	REQUIRE(filter.median() == 15);

	filter.input(18); // [3 7 18 20 30]
	REQUIRE(filter.median() == 18);

	filter.input(0); // [0 3 7 18 30]
	REQUIRE(filter.median() == 7);
}
