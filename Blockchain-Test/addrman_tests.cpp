// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <catch2/catch.hpp>

#include "addrman.h"
#include <string>
#include "random.h"

using namespace std;

class CAddrManTest : public CAddrMan {};

TEST_CASE("addrman_simple")
{
	CAddrManTest addrman;

	// Set addrman addr placement to be deterministic.
	addrman.MakeDeterministic();

	CNetAddr source = CNetAddr("252.2.2.2:8333");

	// Test 1: Does Addrman respond correctly when empty.
	REQUIRE(addrman.size() == 0);
	CAddrInfo addr_null = addrman.Select();
	REQUIRE(addr_null.ToString() == "[::]:0");

	// Test 2: Does Addrman::Add work as expected.
	CService addr1 = CService("250.1.1.1:8333");
	addrman.Add(CAddress(addr1), source);
	REQUIRE(addrman.size() == 1);
	CAddrInfo addr_ret1 = addrman.Select();
	REQUIRE(addr_ret1.ToString() == "250.1.1.1:8333");

	// Test 3: Does IP address deduplication work correctly. 
	//  Expected dup IP should not be added.
	CService addr1_dup = CService("250.1.1.1:8333");
	addrman.Add(CAddress(addr1_dup), source);
	REQUIRE(addrman.size() == 1);


	// Test 5: New table has one addr and we add a diff addr we should
	//  have two addrs.
	CService addr2 = CService("250.1.1.2:8333");
	addrman.Add(CAddress(addr2), source);
	REQUIRE(addrman.size() == 2);

	// Test 6: AddrMan::Clear() should empty the new table. 
	addrman.Clear();
	REQUIRE(addrman.size() == 0);
	CAddrInfo addr_null2 = addrman.Select();
	REQUIRE(addr_null2.ToString() == "[::]:0");
}

TEST_CASE("addrman_ports")
{
	CAddrManTest addrman;

	// Set addrman addr placement to be deterministic.
	addrman.MakeDeterministic();

	CNetAddr source = CNetAddr("252.2.2.2:8333");

	REQUIRE(addrman.size() == 0);

	// Test 7; Addr with same IP but diff port does not replace existing addr.
	CService addr1 = CService("250.1.1.1:8333");
	addrman.Add(CAddress(addr1), source);
	REQUIRE(addrman.size() == 1);

	CService addr1_port = CService("250.1.1.1:8334");
	addrman.Add(CAddress(addr1_port), source);
	REQUIRE(addrman.size() == 1);
	CAddrInfo addr_ret2 = addrman.Select();
	REQUIRE(addr_ret2.ToString() == "250.1.1.1:8333");

	// Test 8: Add same IP but diff port to tried table, it doesn't get added.
	//  Perhaps this is not ideal behavior but it is the current behavior.
	addrman.Good(CAddress(addr1_port));
	REQUIRE(addrman.size() == 1);
	bool newOnly = true;
	CAddrInfo addr_ret3 = addrman.Select(newOnly);
	REQUIRE(addr_ret3.ToString() == "250.1.1.1:8333");
}


TEST_CASE("addrman_select")
{
	CAddrManTest addrman;

	// Set addrman addr placement to be deterministic.
	addrman.MakeDeterministic();

	CNetAddr source = CNetAddr("252.2.2.2:8333");

	// Test 9: Select from new with 1 addr in new.
	CService addr1 = CService("250.1.1.1:8333");
	addrman.Add(CAddress(addr1), source);
	REQUIRE(addrman.size() == 1);

	bool newOnly = true;
	CAddrInfo addr_ret1 = addrman.Select(newOnly);
	REQUIRE(addr_ret1.ToString() == "250.1.1.1:8333");


	// Test 10: move addr to tried, select from new expected nothing returned.
	addrman.Good(CAddress(addr1));
	REQUIRE(addrman.size() == 1);
	CAddrInfo addr_ret2 = addrman.Select(newOnly);
	REQUIRE(addr_ret2.ToString() == "[::]:0");

	CAddrInfo addr_ret3 = addrman.Select();
	REQUIRE(addr_ret3.ToString() == "250.1.1.1:8333");
}

TEST_CASE("addrman_new_collisions")
{
	CAddrManTest addrman;

	// Set addrman addr placement to be deterministic.
	addrman.MakeDeterministic();

	CNetAddr source = CNetAddr("252.2.2.2:8333");

	REQUIRE(addrman.size() == 0);

	for (unsigned int i = 1; i < 4; i++) {
		CService addr = CService("250.1.1." + std::to_string(i));
		addrman.Add(CAddress(addr), source);

		//Test 11: No collision in new table yet.
		REQUIRE(addrman.size() == i);
	}

	//Test 12: new table collision!
	CService addr1 = CService("250.1.1.4");
	addrman.Add(CAddress(addr1), source);
	REQUIRE(addrman.size() == 4);

	CService addr2 = CService("250.1.1.5");
	addrman.Add(CAddress(addr2), source);
	REQUIRE(addrman.size() == 5);
}

TEST_CASE("addrman_tried_collisions")
{
	CAddrManTest addrman;

	// Set addrman addr placement to be deterministic.
	addrman.MakeDeterministic();

	CNetAddr source = CNetAddr("252.2.2.2:8333");

	REQUIRE(addrman.size() == 0);

	for (unsigned int i = 1; i < 75; i++) {
		CService addr = CService("250.1.1." + std::to_string(i));
		addrman.Add(CAddress(addr), source);
		addrman.Good(CAddress(addr));

		//Test 13: No collision in tried table yet.
		REQUIRE(addrman.size() == i);
	}

	//Test 14: tried table collision!
	CService addr1 = CService("250.1.1.76");
	addrman.Add(CAddress(addr1), source);
	REQUIRE(addrman.size() == 75);

	CService addr2 = CService("250.1.1.77");
	addrman.Add(CAddress(addr2), source);
	REQUIRE(addrman.size() == 76);
}