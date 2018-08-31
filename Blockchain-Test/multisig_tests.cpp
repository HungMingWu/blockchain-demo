// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <catch2/catch.hpp>

#include "key.h"
#include "keystore.h"
#include "policy/policy.h"
#include "script/script.h"
#include "script/script_error.h"
#include "script/interpreter.h"
#include "script/sign.h"
#include "uint256.h"

#ifdef ENABLE_WALLET
#include "wallet/wallet_ismine.h"
#endif

using namespace std;

typedef vector<unsigned char> valtype;

CScript
sign_multisig(CScript scriptPubKey, vector<CKey> keys, CTransaction transaction, int whichIn)
{
	uint256 hash = SignatureHash(scriptPubKey, transaction, whichIn, SIGHASH_ALL);

	CScript result;
	result << OP_0; // CHECKMULTISIG bug workaround
	for (const CKey &key : keys)
	{
		vector<unsigned char> vchSig;
		REQUIRE(key.Sign(hash, vchSig));
		vchSig.push_back((unsigned char)SIGHASH_ALL);
		result << vchSig;
	}
	return result;
}

TEST_CASE("multisig_verify")
{
	unsigned int flags = SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_STRICTENC;

	ScriptError err;
	CKey key[4];
	for (int i = 0; i < 4; i++)
		key[i].MakeNewKey(true);

	CScript a_and_b;
	a_and_b << OP_2 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_2 << OP_CHECKMULTISIG;

	CScript a_or_b;
	a_or_b << OP_1 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_2 << OP_CHECKMULTISIG;

	CScript escrow;
	escrow << OP_2 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << ToByteVector(key[2].GetPubKey()) << OP_3 << OP_CHECKMULTISIG;

	CMutableTransaction txFrom;  // Funding transaction
	txFrom.vout.resize(3);
	txFrom.vout[0].scriptPubKey = a_and_b;
	txFrom.vout[1].scriptPubKey = a_or_b;
	txFrom.vout[2].scriptPubKey = escrow;

	CMutableTransaction txTo[3]; // Spending transaction
	for (int i = 0; i < 3; i++)
	{
		txTo[i].vin.resize(1);
		txTo[i].vout.resize(1);
		txTo[i].vin[0].prevout.n = i;
		txTo[i].vin[0].prevout.hash = txFrom.GetHash();
		txTo[i].vout[0].nValue = 1;
	}

	vector<CKey> keys;
	CScript s;

	// Test a AND b:
	keys.assign(1, key[0]);
	keys.push_back(key[1]);
	s = sign_multisig(a_and_b, keys, txTo[0], 0);
	REQUIRE(VerifyScript(s, a_and_b, flags, MutableTransactionSignatureChecker(&txTo[0], 0), &err));
	SECTION(ScriptErrorString(err)) {
		REQUIRE(err == SCRIPT_ERR_OK);
	}

	for (int i = 0; i < 4; i++)
	{
		keys.assign(1, key[i]);
		s = sign_multisig(a_and_b, keys, txTo[0], 0);
		SECTION(strprintf("a&b 1: %d", i)) {
			REQUIRE(!VerifyScript(s, a_and_b, flags, MutableTransactionSignatureChecker(&txTo[0], 0), &err));
		}
		SECTION(ScriptErrorString(err)) {
			REQUIRE(err == SCRIPT_ERR_INVALID_STACK_OPERATION);
		}

		keys.assign(1, key[1]);
		keys.push_back(key[i]);
		s = sign_multisig(a_and_b, keys, txTo[0], 0);
		SECTION(strprintf("a&b 2: %d", i)) {
			REQUIRE(!VerifyScript(s, a_and_b, flags, MutableTransactionSignatureChecker(&txTo[0], 0), &err));
		}
		SECTION(ScriptErrorString(err)) {
			REQUIRE(err == SCRIPT_ERR_EVAL_FALSE);
		}
	}

	// Test a OR b:
	for (int i = 0; i < 4; i++)
	{
		keys.assign(1, key[i]);
		s = sign_multisig(a_or_b, keys, txTo[1], 0);
		if (i == 0 || i == 1)
		{
			SECTION(strprintf("a|b: %d", i)) {
				REQUIRE(VerifyScript(s, a_or_b, flags, MutableTransactionSignatureChecker(&txTo[1], 0), &err));
			}
			SECTION(ScriptErrorString(err)) {
				REQUIRE(err == SCRIPT_ERR_OK);
			}
		}
		else
		{
			SECTION(strprintf("a|b: %d", i)) {
				REQUIRE(!VerifyScript(s, a_or_b, flags, MutableTransactionSignatureChecker(&txTo[1], 0), &err));
			}
			SECTION(ScriptErrorString(err)) {
				REQUIRE(err == SCRIPT_ERR_EVAL_FALSE);
			}
		}
	}
	s.clear();
	s << OP_0 << OP_1;
	REQUIRE(!VerifyScript(s, a_or_b, flags, MutableTransactionSignatureChecker(&txTo[1], 0), &err));
	SECTION(ScriptErrorString(err)) {
		REQUIRE(err == SCRIPT_ERR_SIG_DER);
	}


	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
		{
			keys.assign(1, key[i]);
			keys.push_back(key[j]);
			s = sign_multisig(escrow, keys, txTo[2], 0);
			if (i < j && i < 3 && j < 3)
			{
				SECTION(strprintf("escrow 1: %d %d", i, j)) {
					REQUIRE(VerifyScript(s, escrow, flags, MutableTransactionSignatureChecker(&txTo[2], 0), &err));
				}
				SECTION(ScriptErrorString(err)) {
					REQUIRE(err == SCRIPT_ERR_OK);
				}
			}
			else
			{
				SECTION(strprintf("escrow 2: %d %d", i, j)) {
					REQUIRE(!VerifyScript(s, escrow, flags, MutableTransactionSignatureChecker(&txTo[2], 0), &err));
				}
				SECTION(ScriptErrorString(err)) {
					REQUIRE(err == SCRIPT_ERR_EVAL_FALSE);
				}
			}
		}
}

TEST_CASE("multisig_IsStandard")
{
	CKey key[4];
	for (int i = 0; i < 4; i++)
		key[i].MakeNewKey(true);

	txnouttype whichType;

	CScript a_and_b;
	a_and_b << OP_2 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_2 << OP_CHECKMULTISIG;
	REQUIRE(::IsStandard(a_and_b, whichType));

	CScript a_or_b;
	a_or_b << OP_1 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_2 << OP_CHECKMULTISIG;
	REQUIRE(::IsStandard(a_or_b, whichType));

	CScript escrow;
	escrow << OP_2 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << ToByteVector(key[2].GetPubKey()) << OP_3 << OP_CHECKMULTISIG;
	REQUIRE(::IsStandard(escrow, whichType));

	CScript one_of_four;
	one_of_four << OP_1 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << ToByteVector(key[2].GetPubKey()) << ToByteVector(key[3].GetPubKey()) << OP_4 << OP_CHECKMULTISIG;
	REQUIRE(!::IsStandard(one_of_four, whichType));

	CScript malformed[6];
	malformed[0] << OP_3 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_2 << OP_CHECKMULTISIG;
	malformed[1] << OP_2 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_3 << OP_CHECKMULTISIG;
	malformed[2] << OP_0 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_2 << OP_CHECKMULTISIG;
	malformed[3] << OP_1 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_0 << OP_CHECKMULTISIG;
	malformed[4] << OP_1 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_CHECKMULTISIG;
	malformed[5] << OP_1 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey());

	for (int i = 0; i < 6; i++)
		REQUIRE(!::IsStandard(malformed[i], whichType));
}

TEST_CASE("multisig_Solver1")
{
	// Tests Solver() that returns lists of keys that are
	// required to satisfy a ScriptPubKey
	//
	// Also tests IsMine() and ExtractDestination()
	//
	// Note: ExtractDestination for the multisignature transactions
	// always returns false for this release, even if you have
	// one key that would satisfy an (a|b) or 2-of-3 keys needed
	// to spend an escrow transaction.
	//
	CBasicKeyStore keystore, emptykeystore, partialkeystore;
	CKey key[3];
	CTxDestination keyaddr[3];
	for (int i = 0; i < 3; i++)
	{
		key[i].MakeNewKey(true);
		keystore.AddKey(key[i]);
		keyaddr[i] = key[i].GetPubKey().GetID();
	}
	partialkeystore.AddKey(key[0]);

	{
		vector<valtype> solutions;
		txnouttype whichType;
		CScript s;
		s << ToByteVector(key[0].GetPubKey()) << OP_CHECKSIG;
		REQUIRE(Solver(s, whichType, solutions));
		REQUIRE(solutions.size() == 1);
		CTxDestination addr;
		REQUIRE(ExtractDestination(s, addr));
		REQUIRE(addr == keyaddr[0]);
#ifdef ENABLE_WALLET
		REQUIRE(IsMine(keystore, s));
		REQUIRE(!IsMine(emptykeystore, s));
#endif
	}
	{
		vector<valtype> solutions;
		txnouttype whichType;
		CScript s;
		s << OP_DUP << OP_HASH160 << ToByteVector(key[0].GetPubKey().GetID()) << OP_EQUALVERIFY << OP_CHECKSIG;
		REQUIRE(Solver(s, whichType, solutions));
		REQUIRE(solutions.size() == 1);
		CTxDestination addr;
		REQUIRE(ExtractDestination(s, addr));
		REQUIRE(addr == keyaddr[0]);
#ifdef ENABLE_WALLET
		REQUIRE(IsMine(keystore, s));
		REQUIRE(!IsMine(emptykeystore, s));
#endif
	}
	{
		vector<valtype> solutions;
		txnouttype whichType;
		CScript s;
		s << OP_2 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_2 << OP_CHECKMULTISIG;
		REQUIRE(Solver(s, whichType, solutions));
		REQUIRE(solutions.size() == 4U);
		CTxDestination addr;
		REQUIRE(!ExtractDestination(s, addr));
#ifdef ENABLE_WALLET
		REQUIRE(IsMine(keystore, s));
		REQUIRE(!IsMine(emptykeystore, s));
		REQUIRE(!IsMine(partialkeystore, s));
#endif
	}
	{
		vector<valtype> solutions;
		txnouttype whichType;
		CScript s;
		s << OP_1 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_2 << OP_CHECKMULTISIG;
		REQUIRE(Solver(s, whichType, solutions));
		REQUIRE(solutions.size() == 4U);
		vector<CTxDestination> addrs;
		int nRequired;
		REQUIRE(ExtractDestinations(s, whichType, addrs, nRequired));
		REQUIRE(addrs[0] == keyaddr[0]);
		REQUIRE(addrs[1] == keyaddr[1]);
		REQUIRE(nRequired == 1);
#ifdef ENABLE_WALLET
		REQUIRE(IsMine(keystore, s));
		REQUIRE(!IsMine(emptykeystore, s));
		REQUIRE(!IsMine(partialkeystore, s));
#endif
	}
	{
		vector<valtype> solutions;
		txnouttype whichType;
		CScript s;
		s << OP_2 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << ToByteVector(key[2].GetPubKey()) << OP_3 << OP_CHECKMULTISIG;
		REQUIRE(Solver(s, whichType, solutions));
		REQUIRE(solutions.size() == 5);
	}
}

TEST_CASE("multisig_Sign")
{
	// Test SignSignature() (and therefore the version of Solver() that signs transactions)
	CBasicKeyStore keystore;
	CKey key[4];
	for (int i = 0; i < 4; i++)
	{
		key[i].MakeNewKey(true);
		keystore.AddKey(key[i]);
	}

	CScript a_and_b;
	a_and_b << OP_2 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_2 << OP_CHECKMULTISIG;

	CScript a_or_b;
	a_or_b << OP_1 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << OP_2 << OP_CHECKMULTISIG;

	CScript escrow;
	escrow << OP_2 << ToByteVector(key[0].GetPubKey()) << ToByteVector(key[1].GetPubKey()) << ToByteVector(key[2].GetPubKey()) << OP_3 << OP_CHECKMULTISIG;

	CMutableTransaction txFrom;  // Funding transaction
	txFrom.vout.resize(3);
	txFrom.vout[0].scriptPubKey = a_and_b;
	txFrom.vout[1].scriptPubKey = a_or_b;
	txFrom.vout[2].scriptPubKey = escrow;

	CMutableTransaction txTo[3]; // Spending transaction
	for (int i = 0; i < 3; i++)
	{
		txTo[i].vin.resize(1);
		txTo[i].vout.resize(1);
		txTo[i].vin[0].prevout.n = i;
		txTo[i].vin[0].prevout.hash = txFrom.GetHash();
		txTo[i].vout[0].nValue = 1;
	}

	for (int i = 0; i < 3; i++)
	{
		SECTION(strprintf("SignSignature %d", i)) {
			REQUIRE(SignSignature(keystore, txFrom, txTo[i], 0));
		}
	}
}