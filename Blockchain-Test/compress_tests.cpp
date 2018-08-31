// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stdint.h>

#include <catch2/catch.hpp>

#include "compressor.h"
#include "util.h"

// amounts 0.00000001 .. 0.00100000
#define NUM_MULTIPLES_UNIT 100000

// amounts 0.01 .. 100.00
#define NUM_MULTIPLES_CENT 10000

// amounts 1 .. 10000
#define NUM_MULTIPLES_1UT 10000

// amounts 50 .. 21000000
#define NUM_MULTIPLES_50UT 420000

bool static TestEncode(uint64_t in) {
	return in == CTxOutCompressor::DecompressAmount(CTxOutCompressor::CompressAmount(in));
}

bool static TestDecode(uint64_t in) {
	return in == CTxOutCompressor::CompressAmount(CTxOutCompressor::DecompressAmount(in));
}

bool static TestPair(uint64_t dec, uint64_t enc) {
	return CTxOutCompressor::CompressAmount(dec) == enc &&
		CTxOutCompressor::DecompressAmount(enc) == dec;
}

TEST_CASE("compress_amounts")
{
	REQUIRE(TestPair(0, 0x0));
	REQUIRE(TestPair(1, 0x1));
	REQUIRE(TestPair(CENT, 0x7));
	REQUIRE(TestPair(COIN, 0x9));
	REQUIRE(TestPair(50 * COIN, 0x32));
	REQUIRE(TestPair(21000000 * COIN, 0x1406f40));

	for (uint64_t i = 1; i <= NUM_MULTIPLES_UNIT; i++)
		REQUIRE(TestEncode(i));

	for (uint64_t i = 1; i <= NUM_MULTIPLES_CENT; i++)
		REQUIRE(TestEncode(i * CENT));

	for (uint64_t i = 1; i <= NUM_MULTIPLES_1UT; i++)
		REQUIRE(TestEncode(i * COIN));

	for (uint64_t i = 1; i <= NUM_MULTIPLES_50UT; i++)
		REQUIRE(TestEncode(i * 50 * COIN));

	for (uint64_t i = 0; i < 100000; i++)
		REQUIRE(TestDecode(i));
}
