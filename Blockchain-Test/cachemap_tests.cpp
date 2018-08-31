// Copyright (c) 2014-2017 The Dash Core developers

#include <catch2/catch.hpp>
#include "main.h"
#include "cachemap.h"
#include "streams.h"

bool Compare(const CacheMap<int, int>& map1, const CacheMap<int, int>& map2)
{
	if (map1.GetMaxSize() != map2.GetMaxSize()) {
		return false;
	}

	if (map1.GetSize() != map2.GetSize()) {
		return false;
	}

	const CacheMap<int, int>::list_t& items1 = map1.GetItemList();
	for (CacheMap<int, int>::list_cit it = items1.begin(); it != items1.end(); ++it) {
		if (!map2.HasKey(it->key)) {
			return false;
		}
		Opt<int> val = map2.Get(it->key);
		if (!val) {
			return false;
		}
		if (it->value != val.get()) {
			return false;
		}
	}

	const CacheMap<int, int>::list_t& items2 = map2.GetItemList();
	for (CacheMap<int, int>::list_cit it = items2.begin(); it != items2.end(); ++it) {
		if (!map1.HasKey(it->key)) {
			return false;
		}
		Opt<int> val = map1.Get(it->key);
		if (!val) {
			return false;
		}
		if (it->value != val.get()) {
			return false;
		}
	}

	return true;
}

TEST_CASE("cachemap_test")
{
	// create a CacheMap limited to 10 items
	CacheMap<int, int> mapTest1(10);

	// check that the max size is 10
	REQUIRE(mapTest1.GetMaxSize() == 10);

	// check that the size is 0
	REQUIRE(mapTest1.GetSize() == 0);

	// insert (-1, -1)
	mapTest1.Insert(-1, -1);

	// make sure that the size is updated
	REQUIRE(mapTest1.GetSize() == 1);

	// make sure the map contains the key
	REQUIRE(mapTest1.HasKey(-1) == true);

	// add 10 items
	for (int i = 0; i < 10; ++i) {
		mapTest1.Insert(i, i);
	}

	// check that the size is 10
	REQUIRE(mapTest1.GetSize() == 10);

	// check that the map contains the expected items
	for (int i = 0; i < 10; ++i) {
		Opt<int> nVal = mapTest1.Get(i);
		REQUIRE(bool(nVal));
		REQUIRE(nVal.get() == i);
	}

	// check that the map no longer contains the first item
	REQUIRE(mapTest1.HasKey(-1) == false);

	// erase an item
	mapTest1.Erase(5);

	// check the size
	REQUIRE(mapTest1.GetSize() == 9);

	// check that the map no longer contains the item
	REQUIRE(mapTest1.HasKey(5) == false);

	// check that the map contains the expected items
	int expected[] = { 0, 1, 2, 3, 4, 6, 7, 8, 9 };
	for (size_t i = 0; i < 9; ++i) {
		int eVal = expected[i];
		Opt<int> nVal = mapTest1.Get(eVal);
		REQUIRE(bool(nVal));
		REQUIRE(nVal.get() == eVal);
	}

	// test serialization
	CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
	ss << mapTest1;

	CacheMap<int, int> mapTest2;
	ss >> mapTest2;

	REQUIRE(Compare(mapTest1, mapTest2));

	// test copy constructor
	CacheMap<int, int> mapTest3(mapTest1);
	REQUIRE(Compare(mapTest1, mapTest3));

	// test assignment operator
	CacheMap<int, int> mapTest4;
	mapTest4 = mapTest1;
	REQUIRE(Compare(mapTest1, mapTest4));
}