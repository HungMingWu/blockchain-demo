// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <catch2/catch.hpp>

#include "consensus/validation.h"
#include "key.h"
#include "main.h"
#include "miner.h"
#include "pubkey.h"
#include "txmempool.h"
#include "random.h"
#include "script/standard.h"
#include "utiltime.h"
#include "test_ulord.h"

static bool
ToMemPool(CMutableTransaction& tx)
{
	LOCK(cs_main);

	CValidationState state = AcceptToMemoryPool(mempool, tx, false, NULL);
	return state.IsValid();
}

TEST_CASE_METHOD(TestChain100Setup, "tx_mempool_block_doublespend")
{
	// Make sure skipping validation of transctions that were
	// validated going into the memory pool does not allow
	// double-spends in blocks to pass validation when they should not.

	CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;

	// Create a double-spend of mature coinbase txn:
	std::vector<CMutableTransaction> spends;
	spends.resize(2);
	for (int i = 0; i < 2; i++)
	{
		spends[i].vin.resize(1);
		spends[i].vin[0].prevout.hash = coinbaseTxns[0].GetHash();
		spends[i].vin[0].prevout.n = 0;
		spends[i].vout.resize(1);
		spends[i].vout[0].nValue = 11 * CENT;
		spends[i].vout[0].scriptPubKey = scriptPubKey;

		// Sign:
		std::vector<unsigned char> vchSig;
		uint256 hash = SignatureHash(scriptPubKey, spends[i], 0, SIGHASH_ALL);
		REQUIRE(coinbaseKey.Sign(hash, vchSig));
		vchSig.push_back((unsigned char)SIGHASH_ALL);
		spends[i].vin[0].scriptSig << vchSig;
	}

	CBlock block;

	// Test 1: block with both of those transactions should be rejected.
	block = CreateAndProcessBlock(spends, scriptPubKey);
	REQUIRE(chainActive.Tip()->GetBlockHash() != block.GetHash());

	// Test 2: ... and should be rejected if spend1 is in the memory pool
	REQUIRE(ToMemPool(spends[0]));
	block = CreateAndProcessBlock(spends, scriptPubKey);
	REQUIRE(chainActive.Tip()->GetBlockHash() != block.GetHash());
	mempool.clear();

	// Test 3: ... and should be rejected if spend2 is in the memory pool
	REQUIRE(ToMemPool(spends[1]));
	block = CreateAndProcessBlock(spends, scriptPubKey);
	REQUIRE(chainActive.Tip()->GetBlockHash() != block.GetHash());
	mempool.clear();

	// Final sanity test: first spend in mempool, second in block, that's OK:
	std::vector<CMutableTransaction> oneSpend;
	oneSpend.push_back(spends[0]);
	REQUIRE(ToMemPool(spends[1]));
	block = CreateAndProcessBlock(oneSpend, scriptPubKey);
	REQUIRE(chainActive.Tip()->GetBlockHash() == block.GetHash());
	// spends[1] should have been removed from the mempool when the
	// block with spends[0] is accepted:
	REQUIRE(mempool.size() == 0);
}
