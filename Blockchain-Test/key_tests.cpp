// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <vector>

#include <catch2/catch.hpp>

#include "key.h"
#include "base58.h"
#include "script/script.h"
#include "uint256.h"
#include "util.h"
#include "utilstrencodings.h"


using namespace std;

static const string strSecret1("Kz4i5iDjrGZEtT2D9UaFihxvjeZLecyG4P61NAU6uCdPp7KDbU3W");
static const string strSecret2("L5fA9sReMcDccgKaoZQGABrdEHoUdJUbTuKZahqaYHAnFzTjgyF2");
static const string strSecret1C("KxSaEMpYWyZhHi4sVaog4AHowJ6PhpmNUPczMVHrvqbUsd4ZUXq7");
static const string strSecret2C("L3hb8FLXYxb7R75jCRoMZhoGdMuEUawq8sf2A14sujYqpNEq76jc");
static const CBitcoinAddress addr1("UXnVjzYwE6GMKTaNLpwFCyo8W3DaPdSF6k");
static const CBitcoinAddress addr2("UZ1nEVHkFXKZtRe5kRQDkAZLCFxTcVARy6");
static const CBitcoinAddress addr1C("UbDEtGfWx8YqyAeC6NCeoFE1DD7HfuU2so");
static const CBitcoinAddress addr2C("UgEMKmskcVkEXBmrUVBFLtbw2taAy5Ey4f");


static const string strAddressBad("UgEMKmskcVkEXBmrUVBFLtbw2taAy5Ey41");


#ifdef KEY_TESTS_DUMPINFO
void dumpKeyInfo(uint256 privkey)
{
	CKey key;
	key.resize(32);
	memcpy(&secret[0], &privkey, 32);
	vector<unsigned char> sec;
	sec.resize(32);
	memcpy(&sec[0], &secret[0], 32);
	printf("  * secret (hex): %s\n", HexStr(sec).c_str());

	for (int nCompressed = 0; nCompressed < 2; nCompressed++)
	{
		bool fCompressed = nCompressed == 1;
		printf("  * %s:\n", fCompressed ? "compressed" : "uncompressed");
		CBitcoinSecret bsecret;
		bsecret.SetSecret(secret, fCompressed);
		printf("    * secret (base58): %s\n", bsecret.ToString().c_str());
		CKey key;
		key.SetSecret(secret, fCompressed);
		vector<unsigned char> vchPubKey = key.GetPubKey();
		printf("    * pubkey (hex): %s\n", HexStr(vchPubKey).c_str());
		printf("    * address (base58): %s\n", CBitcoinAddress(vchPubKey).ToString().c_str());
	}
}
#endif

TEST_CASE("key_test1")
{
	SelectParams(CBaseChainParams::MAIN);

	CBitcoinSecret bsecret1, bsecret2, bsecret1C, bsecret2C, baddress1;
	REQUIRE(bsecret1.SetString(strSecret1));
	REQUIRE(bsecret2.SetString(strSecret2));
	REQUIRE(bsecret1C.SetString(strSecret1C));
	REQUIRE(bsecret2C.SetString(strSecret2C));
	REQUIRE(!baddress1.SetString(strAddressBad));

	CKey key1 = bsecret1.GetKey();
	CKey key2 = bsecret2.GetKey();

	CKey key1C = bsecret1C.GetKey();
	REQUIRE(key1C.IsCompressed() == true);
	CKey key2C = bsecret2C.GetKey();
	REQUIRE(key2C.IsCompressed() == true);

	CPubKey pubkey1 = key1.GetPubKey();
	CPubKey pubkey2 = key2.GetPubKey();
	CPubKey pubkey1C = key1C.GetPubKey();
	CPubKey pubkey2C = key2C.GetPubKey();

	REQUIRE(key1.VerifyPubKey(pubkey1));
	REQUIRE(!key1.VerifyPubKey(pubkey1C));
	REQUIRE(!key1.VerifyPubKey(pubkey2));
	REQUIRE(!key1.VerifyPubKey(pubkey2C));

	REQUIRE(!key1C.VerifyPubKey(pubkey1));
	REQUIRE(key1C.VerifyPubKey(pubkey1C));
	REQUIRE(!key1C.VerifyPubKey(pubkey2));
	REQUIRE(!key1C.VerifyPubKey(pubkey2C));

	REQUIRE(!key2.VerifyPubKey(pubkey1));
	REQUIRE(!key2.VerifyPubKey(pubkey1C));
	REQUIRE(key2.VerifyPubKey(pubkey2));
	REQUIRE(!key2.VerifyPubKey(pubkey2C));

	REQUIRE(!key2C.VerifyPubKey(pubkey1));
	REQUIRE(!key2C.VerifyPubKey(pubkey1C));
	REQUIRE(!key2C.VerifyPubKey(pubkey2));
	REQUIRE(key2C.VerifyPubKey(pubkey2C));

	REQUIRE(bool(addr1.Get() == CTxDestination(pubkey1.GetID())));
	REQUIRE(bool(addr2.Get() == CTxDestination(pubkey2.GetID())));
	REQUIRE(bool(addr1C.Get() == CTxDestination(pubkey1C.GetID())));
	REQUIRE(bool(addr2C.Get() == CTxDestination(pubkey2C.GetID())));

	for (int n = 0; n < 16; n++)
	{
		string strMsg = strprintf("Very secret message %i: 11", n);
		uint256 hashMsg = Hash(strMsg.begin(), strMsg.end());

		// normal signatures

		vector<unsigned char> sign1, sign2, sign1C, sign2C;

		REQUIRE(key1.Sign(hashMsg, sign1));
		REQUIRE(key2.Sign(hashMsg, sign2));
		REQUIRE(key1C.Sign(hashMsg, sign1C));
		REQUIRE(key2C.Sign(hashMsg, sign2C));

		REQUIRE(pubkey1.Verify(hashMsg, sign1));
		REQUIRE(!pubkey1.Verify(hashMsg, sign2));
		// REQUIRE( pubkey1.Verify(hashMsg, sign1C));
		REQUIRE(!pubkey1.Verify(hashMsg, sign2C));

		REQUIRE(!pubkey2.Verify(hashMsg, sign1));
		REQUIRE(pubkey2.Verify(hashMsg, sign2));
		REQUIRE(!pubkey2.Verify(hashMsg, sign1C));
		//REQUIRE( pubkey2.Verify(hashMsg, sign2C));

		//REQUIRE( pubkey1C.Verify(hashMsg, sign1));
		REQUIRE(!pubkey1C.Verify(hashMsg, sign2));
		REQUIRE(pubkey1C.Verify(hashMsg, sign1C));
		REQUIRE(!pubkey1C.Verify(hashMsg, sign2C));

		REQUIRE(!pubkey2C.Verify(hashMsg, sign1));
		// REQUIRE( pubkey2C.Verify(hashMsg, sign2));
		REQUIRE(!pubkey2C.Verify(hashMsg, sign1C));
		REQUIRE(pubkey2C.Verify(hashMsg, sign2C));

		// compact signatures (with key recovery)

		vector<unsigned char> csign1, csign2, csign1C, csign2C;

		REQUIRE(key1.SignCompact(hashMsg, csign1));
		REQUIRE(key2.SignCompact(hashMsg, csign2));
		REQUIRE(key1C.SignCompact(hashMsg, csign1C));
		REQUIRE(key2C.SignCompact(hashMsg, csign2C));

		CPubKey rkey1, rkey2, rkey1C, rkey2C;

		REQUIRE(rkey1.RecoverCompact(hashMsg, csign1));
		REQUIRE(rkey2.RecoverCompact(hashMsg, csign2));
		REQUIRE(rkey1C.RecoverCompact(hashMsg, csign1C));
		REQUIRE(rkey2C.RecoverCompact(hashMsg, csign2C));

		REQUIRE(rkey1 == pubkey1);
		REQUIRE(rkey2 == pubkey2);
		REQUIRE(rkey1C == pubkey1C);
		REQUIRE(rkey2C == pubkey2C);
	}
}