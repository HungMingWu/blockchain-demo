// Copyright (c) 2014-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <catch2/catch.hpp>
#include "chain.h"
#include "random.h"
#include "util.h"


#include <vector>

constexpr auto SKIPLIST_LENGTH = 300000;

TEST_CASE("skiplist_test")
{
	std::vector<CBlockIndex> vIndex(SKIPLIST_LENGTH);

	for (int i = 0; i < SKIPLIST_LENGTH; i++) {
		vIndex[i].nHeight = i;
		vIndex[i].pprev = (i == 0) ? NULL : &vIndex[i - 1];
		vIndex[i].BuildSkip();
	}

	for (int i = 0; i < SKIPLIST_LENGTH; i++) {
		if (i > 0) {
			REQUIRE(vIndex[i].pskip == &vIndex[vIndex[i].pskip->nHeight]);
			REQUIRE(vIndex[i].pskip->nHeight < i);
		}
		else {
			REQUIRE(vIndex[i].pskip == NULL);
		}
	}

	for (int i = 0; i < 1000; i++) {
		int from = insecure_rand() % (SKIPLIST_LENGTH - 1);
		int to = insecure_rand() % (from + 1);

		REQUIRE(vIndex[SKIPLIST_LENGTH - 1].GetAncestor(from) == &vIndex[from]);
		REQUIRE(vIndex[from].GetAncestor(to) == &vIndex[to]);
		REQUIRE(vIndex[from].GetAncestor(0) == &vIndex[0]);
	}
}

TEST_CASE("getlocator_test")
{
	// Build a main chain 100000 blocks long.
	std::vector<uint256> vHashMain(100000);
	std::vector<CBlockIndex> vBlocksMain(100000);
	for (unsigned int i = 0; i < vBlocksMain.size(); i++) {
		vHashMain[i] = ArithToUint256(i); // Set the hash equal to the height, so we can quickly check the distances.
		vBlocksMain[i].nHeight = i;
		vBlocksMain[i].pprev = i ? &vBlocksMain[i - 1] : NULL;
		vBlocksMain[i].phashBlock = &vHashMain[i];
		vBlocksMain[i].BuildSkip();
		REQUIRE((int)UintToArith256(vBlocksMain[i].GetBlockHash()).GetLow64() == vBlocksMain[i].nHeight);
		REQUIRE((vBlocksMain[i].pprev == NULL || vBlocksMain[i].nHeight == vBlocksMain[i].pprev->nHeight + 1));
	}

	// Build a branch that splits off at block 49999, 50000 blocks long.
	std::vector<uint256> vHashSide(50000);
	std::vector<CBlockIndex> vBlocksSide(50000);
	for (unsigned int i = 0; i < vBlocksSide.size(); i++) {
		vHashSide[i] = ArithToUint256(i + 50000 + (arith_uint256(1) << 128)); // Add 1<<128 to the hashes, so GetLow64() still returns the height.
		vBlocksSide[i].nHeight = i + 50000;
		vBlocksSide[i].pprev = i ? &vBlocksSide[i - 1] : &vBlocksMain[49999];
		vBlocksSide[i].phashBlock = &vHashSide[i];
		vBlocksSide[i].BuildSkip();
		REQUIRE((int)UintToArith256(vBlocksSide[i].GetBlockHash()).GetLow64() == vBlocksSide[i].nHeight);
		REQUIRE((vBlocksSide[i].pprev == NULL || vBlocksSide[i].nHeight == vBlocksSide[i].pprev->nHeight + 1));
	}

	// Build a CChain for the main branch.
	CChain chain;
	chain.SetTip(&vBlocksMain.back());

	// Test 100 random starting points for locators.
	for (int n = 0; n < 100; n++) {
		int r = insecure_rand() % 150000;
		CBlockIndex* tip = (r < 100000) ? &vBlocksMain[r] : &vBlocksSide[r - 100000];
		CBlockLocator locator = chain.GetLocator(tip);

		// The first result must be the block itself, the last one must be genesis.
		REQUIRE(locator.vHave.front() == tip->GetBlockHash());
		REQUIRE(locator.vHave.back() == vBlocksMain[0].GetBlockHash());

		// Entries 1 through 11 (inclusive) go back one step each.
		for (unsigned int i = 1; i < 12 && i < locator.vHave.size() - 1; i++) {
			REQUIRE(UintToArith256(locator.vHave[i]).GetLow64() == tip->nHeight - i);
		}

		// The further ones (excluding the last one) go back with exponential steps.
		unsigned int dist = 2;
		for (unsigned int i = 12; i < locator.vHave.size() - 1; i++) {
			REQUIRE(UintToArith256(locator.vHave[i - 1]).GetLow64() - UintToArith256(locator.vHave[i]).GetLow64() == dist);
			dist *= 2;
		}
	}
}