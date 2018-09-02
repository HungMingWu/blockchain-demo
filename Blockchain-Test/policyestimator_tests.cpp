// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <catch2/catch.hpp>

#include "policy/fees.h"
#include "txmempool.h"
#include "uint256.h"
#include "util.h"
#include "test_ulord.h"

TEST_CASE("BlockPolicyEstimates")
{
	CTxMemPool mpool(CFeeRate(10000)); // we have 10x higher fee
	TestMemPoolEntryHelper entry;
	CAmount basefee(20000); // we have 10x higher fee
	double basepri = 1e6;
	CAmount deltaFee(1000); // we have 10x higher fee
	double deltaPri = 5e5;
	std::vector<CAmount> feeV[2];
	std::vector<double> priV[2];

	// Populate vectors of increasing fees or priorities
	for (int j = 0; j < 10; j++) {
		//V[0] is for fee transactions
		feeV[0].push_back(basefee * (j + 1));
		priV[0].push_back(0);
		//V[1] is for priority transactions
		feeV[1].push_back(CAmount(0));
		priV[1].push_back(basepri * pow(10, j + 1));
	}

	// Store the hashes of transactions that have been
	// added to the mempool by their associate fee/pri
	// txHashes[j] is populated with transactions either of
	// fee = basefee * (j+1)  OR  pri = 10^6 * 10^(j+1)
	std::vector<uint256> txHashes[10];

	// Create a transaction template
	CScript garbage;
	for (unsigned int i = 0; i < 128; i++)
		garbage.push_back('X');
	CMutableTransaction tx;
	std::list<CTransaction> dummyConflicted;
	tx.vin.resize(1);
	tx.vin[0].scriptSig = garbage;
	tx.vout.resize(1);
	tx.vout[0].nValue = 0LL;
	CFeeRate baseRate(basefee, ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION));

	// Create a fake block
	std::vector<CTransaction> block;
	int blocknum = 0;

	// Loop through 200 blocks
	// At a decay .998 and 4 fee transactions per block
	// This makes the tx count about 1.33 per bucket, above the 1 threshold
	while (blocknum < 200) {
		for (int j = 0; j < 10; j++) { // For each fee/pri multiple
			for (int k = 0; k < 5; k++) { // add 4 fee txs for every priority tx
				tx.vin[0].prevout.n = 10000 * blocknum + 100 * j + k; // make transaction unique
				uint256 hash = tx.GetHash();
				mpool.addUnchecked(hash, entry.Fee(feeV[k / 4][j]).Time(GetTime()).Priority(priV[k / 4][j]).Height(blocknum).FromTx(tx, &mpool));
				txHashes[j].push_back(hash);
			}
		}
		//Create blocks where higher fee/pri txs are included more often
		for (int h = 0; h <= blocknum % 10; h++) {
			// 10/10 blocks add highest fee/pri transactions
			// 9/10 blocks add 2nd highest and so on until ...
			// 1/10 blocks add lowest fee/pri transactions
			while (txHashes[9 - h].size()) {
				Opt<CTransaction> btx = mpool.lookup(txHashes[9 - h].back());
				if (btx)
					block.push_back(btx.get());
				txHashes[9 - h].pop_back();
			}
		}
		dummyConflicted = mpool.removeForBlock(block, ++blocknum);
		block.clear();
		if (blocknum == 30) {
			// At this point we should need to combine 5 buckets to get enough data points
			// So estimateFee(1,2,3) should fail and estimateFee(4) should return somewhere around
			// 8*baserate.  estimateFee(4) %'s are 100,100,100,100,90 = average 98%
			REQUIRE(mpool.estimateFee(1) == CFeeRate(0));
			REQUIRE(mpool.estimateFee(2) == CFeeRate(0));
			REQUIRE(mpool.estimateFee(3) == CFeeRate(0));
			REQUIRE(mpool.estimateFee(4).GetFeePerK() < 8 * baseRate.GetFeePerK() + deltaFee);
			REQUIRE(mpool.estimateFee(4).GetFeePerK() > 8 * baseRate.GetFeePerK() - deltaFee);
			int answerFound;
			REQUIRE((mpool.estimateSmartFee(1, &answerFound) == mpool.estimateFee(4) && answerFound == 4));
			REQUIRE((mpool.estimateSmartFee(3, &answerFound) == mpool.estimateFee(4) && answerFound == 4));
			REQUIRE((mpool.estimateSmartFee(4, &answerFound) == mpool.estimateFee(4) && answerFound == 4));
			REQUIRE((mpool.estimateSmartFee(8, &answerFound) == mpool.estimateFee(8) && answerFound == 8));
		}
	}

	std::vector<CAmount> origFeeEst;
	std::vector<double> origPriEst;
	// Highest feerate is 10*baseRate and gets in all blocks,
	// second highest feerate is 9*baseRate and gets in 9/10 blocks = 90%,
	// third highest feerate is 8*base rate, and gets in 8/10 blocks = 80%,
	// so estimateFee(1) should return 10*baseRate.
	// Second highest feerate has 100% chance of being included by 2 blocks,
	// so estimateFee(2) should return 9*baseRate etc...
	for (int i = 1; i < 10; i++) {
		origFeeEst.push_back(mpool.estimateFee(i).GetFeePerK());
		origPriEst.push_back(mpool.estimatePriority(i));
		if (i > 1) { // Fee estimates should be monotonically decreasing
			REQUIRE(origFeeEst[i - 1] <= origFeeEst[i - 2]);
			REQUIRE(origPriEst[i - 1] <= origPriEst[i - 2]);
		}
		int mult = 11 - i;
		REQUIRE(origFeeEst[i - 1] < mult*baseRate.GetFeePerK() + deltaFee);
		REQUIRE(origFeeEst[i - 1] > mult*baseRate.GetFeePerK() - deltaFee);
		REQUIRE(origPriEst[i - 1] < pow(10, mult) * basepri + deltaPri);
		REQUIRE(origPriEst[i - 1] > pow(10, mult) * basepri - deltaPri);
	}

	// Mine 50 more blocks with no transactions happening, estimates shouldn't change
	// We haven't decayed the moving average enough so we still have enough data points in every bucket
	while (blocknum < 250)
		dummyConflicted = mpool.removeForBlock(block, ++blocknum);

	for (int i = 1; i < 10; i++) {
		REQUIRE(mpool.estimateFee(i).GetFeePerK() < origFeeEst[i - 1] + deltaFee);
		REQUIRE(mpool.estimateFee(i).GetFeePerK() > origFeeEst[i - 1] - deltaFee);
		REQUIRE(mpool.estimatePriority(i) < origPriEst[i - 1] + deltaPri);
		REQUIRE(mpool.estimatePriority(i) > origPriEst[i - 1] - deltaPri);
	}


	// Mine 15 more blocks with lots of transactions happening and not getting mined
	// Estimates should go up
	while (blocknum < 265) {
		for (int j = 0; j < 10; j++) { // For each fee/pri multiple
			for (int k = 0; k < 5; k++) { // add 4 fee txs for every priority tx
				tx.vin[0].prevout.n = 10000 * blocknum + 100 * j + k;
				uint256 hash = tx.GetHash();
				mpool.addUnchecked(hash, entry.Fee(feeV[k / 4][j]).Time(GetTime()).Priority(priV[k / 4][j]).Height(blocknum).FromTx(tx, &mpool));
				txHashes[j].push_back(hash);
			}
		}
		dummyConflicted = mpool.removeForBlock(block, ++blocknum);
	}

	int answerFound;
	for (int i = 1; i < 10; i++) {
		REQUIRE((mpool.estimateFee(i) == CFeeRate(0) || mpool.estimateFee(i).GetFeePerK() > origFeeEst[i - 1] - deltaFee));
		REQUIRE((mpool.estimateSmartFee(i, &answerFound).GetFeePerK() > origFeeEst[answerFound - 1] - deltaFee));
		REQUIRE((mpool.estimatePriority(i) == -1 || mpool.estimatePriority(i) > origPriEst[i - 1] - deltaPri));
		REQUIRE((mpool.estimateSmartPriority(i, &answerFound) > origPriEst[answerFound - 1] - deltaPri));
	}

	// Mine all those transactions
	// Estimates should still not be below original
	for (int j = 0; j < 10; j++) {
		while (txHashes[j].size()) {
			Opt<CTransaction> btx = mpool.lookup(txHashes[j].back());
			if (btx)
				block.push_back(btx.get());
			txHashes[j].pop_back();
		}
	}
	dummyConflicted = mpool.removeForBlock(block, 265);
	block.clear();
	for (int i = 1; i < 10; i++) {
		REQUIRE(mpool.estimateFee(i).GetFeePerK() > origFeeEst[i - 1] - deltaFee);
		REQUIRE(mpool.estimatePriority(i) > origPriEst[i - 1] - deltaPri);
	}

	// Mine 200 more blocks where everything is mined every block
	// Estimates should be below original estimates
	while (blocknum < 465) {
		for (int j = 0; j < 10; j++) { // For each fee/pri multiple
			for (int k = 0; k < 5; k++) { // add 4 fee txs for every priority tx
				tx.vin[0].prevout.n = 10000 * blocknum + 100 * j + k;
				uint256 hash = tx.GetHash();
				mpool.addUnchecked(hash, entry.Fee(feeV[k / 4][j]).Time(GetTime()).Priority(priV[k / 4][j]).Height(blocknum).FromTx(tx, &mpool));
				Opt<CTransaction> btx = mpool.lookup(hash);
				if (btx)
					block.push_back(btx.get());
			}
		}
		dummyConflicted = mpool.removeForBlock(block, ++blocknum);
		block.clear();
	}
	for (int i = 1; i < 10; i++) {
		REQUIRE(mpool.estimateFee(i).GetFeePerK() < origFeeEst[i - 1] - deltaFee);
		REQUIRE(mpool.estimatePriority(i) < origPriEst[i - 1] - deltaPri);
	}

	// Test that if the mempool is limited, estimateSmartFee won't return a value below the mempool min fee
	// and that estimateSmartPriority returns essentially an infinite value
	mpool.addUnchecked(tx.GetHash(), entry.Fee(feeV[0][5]).Time(GetTime()).Priority(priV[1][5]).Height(blocknum).FromTx(tx, &mpool));
	// evict that transaction which should set a mempool min fee of minRelayTxFee + feeV[0][5]
	mpool.TrimToSize(1);
	REQUIRE(mpool.GetMinFee(1).GetFeePerK() > feeV[0][5]);
	for (int i = 1; i < 10; i++) {
		REQUIRE(mpool.estimateSmartFee(i).GetFeePerK() >= mpool.estimateFee(i).GetFeePerK());
		REQUIRE(mpool.estimateSmartFee(i).GetFeePerK() >= mpool.GetMinFee(1).GetFeePerK());
		REQUIRE(mpool.estimateSmartPriority(i) == INF_PRIORITY);
	}
}
