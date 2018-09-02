// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <catch2/catch.hpp>

#include "script/script.h"
#include "uint256.h"

using namespace std;

TEST_CASE("IsPayToPublicKeyHash")
{
	// Test CScript::IsPayToPublicKeyHash()
	uint160 dummy;
	CScript p2pkh;
	p2pkh << OP_DUP << OP_HASH160 << ToByteVector(dummy) << OP_EQUALVERIFY << OP_CHECKSIG;
	REQUIRE(p2pkh.IsPayToPublicKeyHash());

	static const unsigned char direct[] = {
		OP_DUP, OP_HASH160, 20, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, OP_EQUALVERIFY, OP_CHECKSIG
	};
	REQUIRE(CScript(direct, direct + sizeof(direct)).IsPayToPublicKeyHash());

	static const unsigned char notp2pkh1[] = {
		OP_DUP, OP_HASH160, 20, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, OP_EQUALVERIFY, OP_CHECKSIG, OP_CHECKSIG
	};
	REQUIRE(!CScript(notp2pkh1, notp2pkh1 + sizeof(notp2pkh1)).IsPayToPublicKeyHash());

	static const unsigned char p2sh[] = {
		OP_HASH160, 20, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, OP_EQUAL
	};
	REQUIRE(!CScript(p2sh, p2sh + sizeof(p2sh)).IsPayToPublicKeyHash());

	static const unsigned char extra[] = {
		OP_DUP, OP_HASH160, 20, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, OP_EQUALVERIFY, OP_CHECKSIG, OP_CHECKSIG
	};
	REQUIRE(!CScript(extra, extra + sizeof(extra)).IsPayToPublicKeyHash());

	static const unsigned char missing[] = {
		OP_HASH160, 20, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, OP_EQUALVERIFY, OP_CHECKSIG, OP_RETURN
	};
	REQUIRE(!CScript(missing, missing + sizeof(missing)).IsPayToPublicKeyHash());

	static const unsigned char missing2[] = {
		OP_DUP, OP_HASH160, 20, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	};
	REQUIRE(!CScript(missing2, missing2 + sizeof(missing)).IsPayToPublicKeyHash());

	static const unsigned char tooshort[] = {
		OP_DUP, OP_HASH160, 2, 0,0, OP_EQUALVERIFY, OP_CHECKSIG
	};
	REQUIRE(!CScript(tooshort, tooshort + sizeof(direct)).IsPayToPublicKeyHash());
}