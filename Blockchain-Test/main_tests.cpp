// Copyright (c) 2014-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// Copyright (c) 2014-2017 The Dash Core developers
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/signals2/signal.hpp>
#include <catch2/catch.hpp>

#include "chainparams.h"
#include "main.h"

static void TestBlockSubsidyHalvings(const Consensus::Params& consensusParams)
{
	// tested in ulord_tests.cpp
	//int maxHalvings = 64;
	//CAmount nInitialSubsidy = 50 * COIN;

	//CAmount nPreviousSubsidy = nInitialSubsidy * 2; // for height == 0
	//BOOST_CHECK_EQUAL(nPreviousSubsidy, nInitialSubsidy * 2);
	//for (int nHalvings = 0; nHalvings < maxHalvings; nHalvings++) {
	//    int nHeight = nHalvings * consensusParams.nSubsidyHalvingInterval;
	//    CAmount nSubsidy = GetBlockSubsidy(nHeight, consensusParams);
	//    REQUIRE(nSubsidy <= nInitialSubsidy);
	//    BOOST_CHECK_EQUAL(nSubsidy, nPreviousSubsidy / 2);
	//    nPreviousSubsidy = nSubsidy;
	//}
	//BOOST_CHECK_EQUAL(GetBlockSubsidy(0, maxHalvings * consensusParams.nSubsidyHalvingInterval, consensusParams), 0);
}

static void TestBlockSubsidyHalvings(int nSubsidyHalvingInterval)
{
	// tested in ulord_tests.cpp
	//Consensus::Params consensusParams;
	//consensusParams.nSubsidyHalvingInterval = nSubsidyHalvingInterval;
	//TestBlockSubsidyHalvings(consensusParams);
}

TEST_CASE("block_subsidy_test")
{
	// tested in ulord_tests.cpp
	//TestBlockSubsidyHalvings(Params(CBaseChainParams::MAIN).GetConsensus()); // As in main
	//TestBlockSubsidyHalvings(150); // As in regtest
	//TestBlockSubsidyHalvings(1000); // Just another interval
}

TEST_CASE("subsidy_limit_test")
{
	// tested in ulord_tests.cpp
	//const Consensus::Params& consensusParams = Params(CBaseChainParams::MAIN).GetConsensus();
	//CAmount nSum = 0;
	//for (int nHeight = 0; nHeight < 14000000; nHeight += 1000) {
	//    /* @TODO fix subsidity, add nBits */
	//    CAmount nSubsidy = GetBlockSubsidy(0, nHeight, consensusParams);
	//    REQUIRE(nSubsidy <= 25 * COIN);
	//    nSum += nSubsidy * 1000;
	//    REQUIRE(MoneyRange(nSum));
	//}
	//BOOST_CHECK_EQUAL(nSum, 1350824726649000ULL);
}

bool ReturnFalse() { return false; }
bool ReturnTrue() { return true; }

TEST_CASE("test_combiner_all")
{
	boost::signals2::signal<bool(), CombinerAll> Test;
	REQUIRE(Test());
	Test.connect(&ReturnFalse);
	REQUIRE(!Test());
	Test.connect(&ReturnTrue);
	REQUIRE(!Test());
	Test.disconnect(&ReturnFalse);
	REQUIRE(Test());
	Test.disconnect(&ReturnTrue);
	REQUIRE(Test());
}
