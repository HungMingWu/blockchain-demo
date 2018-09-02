// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <catch2/catch.hpp>
#include "key.h"

TEST_CASE("basic_sanity")
{
	SECTION("openssl ECC test") {
		REQUIRE(ECC_InitSanityCheck() == true);
	}
}