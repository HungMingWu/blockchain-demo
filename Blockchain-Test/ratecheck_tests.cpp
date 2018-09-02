// Copyright (c) 2014-2017 The Dash Core developers

#include <catch2/catch.hpp>
#include "governance.h"


TEST_CASE("ratecheck_test")
{
	CRateCheckBuffer buffer;

	REQUIRE(buffer.GetCount() == 0);
	REQUIRE(buffer.GetMinTimestamp() == numeric_limits<int64_t>::max());
	REQUIRE(buffer.GetMaxTimestamp() == 0);
	REQUIRE(buffer.GetRate() == 0.0);

	buffer.AddTimestamp(1);

	std::cout << "buffer.GetMinTimestamp() = " << buffer.GetMinTimestamp() << std::endl;

	REQUIRE(buffer.GetCount() == 1);
	REQUIRE(buffer.GetMinTimestamp() == 1);
	REQUIRE(buffer.GetMaxTimestamp() == 1);
	REQUIRE(buffer.GetRate() == 0.0);

	buffer.AddTimestamp(2);
	REQUIRE(buffer.GetCount() == 2);
	REQUIRE(buffer.GetMinTimestamp() == 1);
	REQUIRE(buffer.GetMaxTimestamp() == 2);
	REQUIRE(fabs(buffer.GetRate() - 2.0) < 1.0e-9);

	buffer.AddTimestamp(3);
	REQUIRE(buffer.GetCount() == 3);
	REQUIRE(buffer.GetMinTimestamp() == 1);
	REQUIRE(buffer.GetMaxTimestamp() == 3);

	int64_t nMin = buffer.GetMinTimestamp();
	int64_t nMax = buffer.GetMaxTimestamp();
	double dRate = buffer.GetRate();

	std::cout << "buffer.GetCount() = " << buffer.GetCount() << std::endl;
	std::cout << "nMin = " << nMin << std::endl;
	std::cout << "nMax = " << nMax << std::endl;
	std::cout << "buffer.GetRate() = " << dRate << std::endl;

	REQUIRE(fabs(buffer.GetRate() - (3.0 / 2.0)) < 1.0e-9);

	buffer.AddTimestamp(4);
	REQUIRE(buffer.GetCount() == 4);
	REQUIRE(buffer.GetMinTimestamp() == 1);
	REQUIRE(buffer.GetMaxTimestamp() == 4);
	REQUIRE(fabs(buffer.GetRate() - (4.0 / 3.0)) < 1.0e-9);

	buffer.AddTimestamp(5);
	REQUIRE(buffer.GetCount() == 5);
	REQUIRE(buffer.GetMinTimestamp() == 1);
	REQUIRE(buffer.GetMaxTimestamp() == 5);
	REQUIRE(fabs(buffer.GetRate() - (5.0 / 4.0)) < 1.0e-9);

	buffer.AddTimestamp(6);
	REQUIRE(buffer.GetCount() == 5);
	REQUIRE(buffer.GetMinTimestamp() == 2);
	REQUIRE(buffer.GetMaxTimestamp() == 6);
	REQUIRE(fabs(buffer.GetRate() - (5.0 / 4.0)) < 1.0e-9);

	CRateCheckBuffer buffer2;

	std::cout << "Before loop tests" << std::endl;
	for (int64_t i = 1; i < 11; ++i) {
		std::cout << "In loop: i = " << i << std::endl;
		buffer2.AddTimestamp(i);
		REQUIRE(buffer2.GetCount() == (i <= 5 ? i : 5));
		REQUIRE(buffer2.GetMinTimestamp() == max(int64_t(1), i - 4));
		REQUIRE(buffer2.GetMaxTimestamp() == i);
	}
}