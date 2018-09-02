// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <catch2/catch.hpp>

#include "streams.h"
#include "support/allocators/zeroafterfree.h"



using namespace std;

TEST_CASE("streams_serializedata_xor")
{
	std::vector<char> in;
	std::vector<char> expected_xor;
	std::vector<unsigned char> key;
	CDataStream ds(in, 0, 0);

	// Degenerate case

	key.push_back('\x00');
	key.push_back('\x00');
	ds.Xor(key);
	REQUIRE(
		std::string(expected_xor.begin(), expected_xor.end()) ==
		std::string(ds.begin(), ds.end()));

	in.push_back('\x0f');
	in.push_back('\xf0');
	expected_xor.push_back('\xf0');
	expected_xor.push_back('\x0f');

	// Single character key

	ds.clear();
	ds.insert(ds.begin(), in.begin(), in.end());
	key.clear();

	key.push_back('\xff');
	ds.Xor(key);
	REQUIRE(
		std::string(expected_xor.begin(), expected_xor.end()) ==
		std::string(ds.begin(), ds.end()));

	// Multi character key

	in.clear();
	expected_xor.clear();
	in.push_back('\xf0');
	in.push_back('\x0f');
	expected_xor.push_back('\x0f');
	expected_xor.push_back('\x00');

	ds.clear();
	ds.insert(ds.begin(), in.begin(), in.end());

	key.clear();
	key.push_back('\xff');
	key.push_back('\x0f');

	ds.Xor(key);
	REQUIRE(
		std::string(expected_xor.begin(), expected_xor.end()) ==
		std::string(ds.begin(), ds.end()));
}
