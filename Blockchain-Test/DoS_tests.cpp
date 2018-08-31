// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Unit tests for denial-of-service detection/prevention code

#include <stdint.h>
#include <catch2/catch.hpp>

#include "chainparams.h"
#include "keystore.h"
#include "main.h"
#include "net.h"
#include "pow.h"
#include "script/sign.h"
#include "serialize.h"
#include "util.h"

// Tests this internal-to-main.cpp method:
extern bool AddOrphanTx(const CTransaction& tx, NodeId peer);
extern void EraseOrphansFor(NodeId peer);
extern unsigned int LimitOrphanTxSize(unsigned int nMaxOrphans);
struct COrphanTx {
	CTransaction tx;
	NodeId fromPeer;
};
extern std::map<uint256, COrphanTx> mapOrphanTransactions;
extern std::map<uint256, std::set<uint256> > mapOrphanTransactionsByPrev;

CService ip(uint32_t i)
{
	struct in_addr s;
	s.s_addr = i;
	return CService(CNetAddr(s), Params().GetDefaultPort());
}


TEST_CASE("DoS_banning")
{
	CNode::ClearBanned();
	CAddress addr1(ip(0xa0b0c001));
	CNode dummyNode1(INVALID_SOCKET, addr1, "", true);
	dummyNode1.nVersion = 1;
	Misbehaving(dummyNode1.GetId(), 100); // Should get banned
	SendMessages(&dummyNode1);
	REQUIRE(CNode::IsBanned(addr1));
	REQUIRE(!CNode::IsBanned(ip(0xa0b0c001 | 0x0000ff00))); // Different IP, not banned

	CAddress addr2(ip(0xa0b0c002));
	CNode dummyNode2(INVALID_SOCKET, addr2, "", true);
	dummyNode2.nVersion = 1;
	Misbehaving(dummyNode2.GetId(), 50);
	SendMessages(&dummyNode2);
	REQUIRE(!CNode::IsBanned(addr2)); // 2 not banned yet...
	REQUIRE(CNode::IsBanned(addr1));  // ... but 1 still should be
	Misbehaving(dummyNode2.GetId(), 50);
	SendMessages(&dummyNode2);
	REQUIRE(CNode::IsBanned(addr2));
}

TEST_CASE("DoS_banscore")
{
	CNode::ClearBanned();
	mapArgs["-banscore"] = "111"; // because 11 is my favorite number
	CAddress addr1(ip(0xa0b0c001));
	CNode dummyNode1(INVALID_SOCKET, addr1, "", true);
	dummyNode1.nVersion = 1;
	Misbehaving(dummyNode1.GetId(), 100);
	SendMessages(&dummyNode1);
	REQUIRE(!CNode::IsBanned(addr1));
	Misbehaving(dummyNode1.GetId(), 10);
	SendMessages(&dummyNode1);
	REQUIRE(!CNode::IsBanned(addr1));
	Misbehaving(dummyNode1.GetId(), 1);
	SendMessages(&dummyNode1);
	REQUIRE(CNode::IsBanned(addr1));
	mapArgs.erase("-banscore");
}

TEST_CASE("DoS_bantime")
{
	CNode::ClearBanned();
	int64_t nStartTime = GetTime();
	SetMockTime(nStartTime); // Overrides future calls to GetTime()

	CAddress addr(ip(0xa0b0c001));
	CNode dummyNode(INVALID_SOCKET, addr, "", true);
	dummyNode.nVersion = 1;

	Misbehaving(dummyNode.GetId(), 100);
	SendMessages(&dummyNode);
	REQUIRE(CNode::IsBanned(addr));

	SetMockTime(nStartTime + 60 * 60);
	REQUIRE(CNode::IsBanned(addr));

	SetMockTime(nStartTime + 60 * 60 * 24 + 1);
	REQUIRE(!CNode::IsBanned(addr));
}

CTransaction RandomOrphan()
{
	std::map<uint256, COrphanTx>::iterator it;
	it = mapOrphanTransactions.lower_bound(GetRandHash());
	if (it == mapOrphanTransactions.end())
		it = mapOrphanTransactions.begin();
	return it->second.tx;
}

TEST_CASE("DoS_mapOrphans")
{
	CKey key;
	key.MakeNewKey(true);
	CBasicKeyStore keystore;
	keystore.AddKey(key);

	// 50 orphan transactions:
	for (int i = 0; i < 50; i++)
	{
		CMutableTransaction tx;
		tx.vin.resize(1);
		tx.vin[0].prevout.n = 0;
		tx.vin[0].prevout.hash = GetRandHash();
		tx.vin[0].scriptSig << OP_1;
		tx.vout.resize(1);
		tx.vout[0].nValue = 1 * CENT;
		tx.vout[0].scriptPubKey = GetScriptForDestination(key.GetPubKey().GetID());

		AddOrphanTx(tx, i);
	}

	// ... and 50 that depend on other orphans:
	for (int i = 0; i < 50; i++)
	{
		CTransaction txPrev = RandomOrphan();

		CMutableTransaction tx;
		tx.vin.resize(1);
		tx.vin[0].prevout.n = 0;
		tx.vin[0].prevout.hash = txPrev.GetHash();
		tx.vout.resize(1);
		tx.vout[0].nValue = 1 * CENT;
		tx.vout[0].scriptPubKey = GetScriptForDestination(key.GetPubKey().GetID());
		SignSignature(keystore, txPrev, tx, 0);

		AddOrphanTx(tx, i);
	}

	// This really-big orphan should be ignored:
	for (int i = 0; i < 10; i++)
	{
		CTransaction txPrev = RandomOrphan();

		CMutableTransaction tx;
		tx.vout.resize(1);
		tx.vout[0].nValue = 1 * CENT;
		tx.vout[0].scriptPubKey = GetScriptForDestination(key.GetPubKey().GetID());
		tx.vin.resize(500);
		for (unsigned int j = 0; j < tx.vin.size(); j++)
		{
			tx.vin[j].prevout.n = j;
			tx.vin[j].prevout.hash = txPrev.GetHash();
		}
		SignSignature(keystore, txPrev, tx, 0);
		// Re-use same signature for other inputs
		// (they don't have to be valid for this test)
		for (unsigned int j = 1; j < tx.vin.size(); j++)
			tx.vin[j].scriptSig = tx.vin[0].scriptSig;

		REQUIRE(!AddOrphanTx(tx, i));
	}

	// Test EraseOrphansFor:
	for (NodeId i = 0; i < 3; i++)
	{
		size_t sizeBefore = mapOrphanTransactions.size();
		EraseOrphansFor(i);
		REQUIRE(mapOrphanTransactions.size() < sizeBefore);
	}

	// Test LimitOrphanTxSize() function:
	LimitOrphanTxSize(40);
	REQUIRE(mapOrphanTransactions.size() <= 40);
	LimitOrphanTxSize(10);
	REQUIRE(mapOrphanTransactions.size() <= 10);
	LimitOrphanTxSize(0);
	REQUIRE(mapOrphanTransactions.empty());
	REQUIRE(mapOrphanTransactionsByPrev.empty());
}
