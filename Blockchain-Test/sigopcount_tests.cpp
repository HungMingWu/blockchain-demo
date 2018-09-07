// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <vector>
#include <catch2/catch.hpp>

#include "pubkey.h"
#include "key.h"
#include "script/script.h"
#include "script/standard.h"
#include "uint256.h"
#include "test_ulord.h"

using namespace std;

// Helpers:
static std::vector<unsigned char>
Serialize(const CScript& s)
{
	std::vector<unsigned char> sSerialized(s.begin(), s.end());
	return sSerialized;
}

TEST_CASE_METHOD(BasicTestingSetup, "GetSigOpCount")
{
	// Test CScript::GetSigOpCount()
	CScript s1;
	REQUIRE(s1.GetSigOpCount(false) == 0U);
	REQUIRE(s1.GetSigOpCount(true) == 0U);

	uint160 dummy;
	s1 << OP_1 << ToByteVector(dummy) << ToByteVector(dummy) << OP_2 << OP_CHECKMULTISIG;
	REQUIRE(s1.GetSigOpCount(true) == 2U);
	s1 << OP_IF << OP_CHECKSIG << OP_ENDIF;
	REQUIRE(s1.GetSigOpCount(true) == 3U);
	REQUIRE(s1.GetSigOpCount(false) == 21U);

	CScript p2sh = GetScriptForDestination(CScriptID(s1));
	CScript scriptSig;
	scriptSig << OP_0 << Serialize(s1);
	REQUIRE(p2sh.GetSigOpCount(scriptSig) == 3U);

	std::vector<CPubKey> keys;
	for (int i = 0; i < 3; i++)
	{
		CKey k;
		k.MakeNewKey(true);
		keys.push_back(k.GetPubKey());
	}
	CScript s2 = GetScriptForMultisig(1, keys);
	REQUIRE(s2.GetSigOpCount(true) == 3U);
	REQUIRE(s2.GetSigOpCount(false) == 20U);

	p2sh = GetScriptForDestination(CScriptID(s2));
	REQUIRE(p2sh.GetSigOpCount(true) == 0U);
	REQUIRE(p2sh.GetSigOpCount(false) == 0U);
	CScript scriptSig2;
	scriptSig2 << OP_1 << ToByteVector(dummy) << ToByteVector(dummy) << Serialize(s2);
	REQUIRE(p2sh.GetSigOpCount(scriptSig2) == 3U);
}