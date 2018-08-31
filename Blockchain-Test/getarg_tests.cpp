// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <catch2/catch.hpp>

#include "util.h"

static void ResetArgs(const std::string& strArg)
{
	std::vector<std::string> vecArg;
	if (strArg.size())
		boost::split(vecArg, strArg, boost::is_space(), boost::token_compress_on);

	// Insert dummy executable name:
	vecArg.insert(vecArg.begin(), "testulord");

	// Convert to char*:
	std::vector<const char*> vecChar;
	for (std::string& s : vecArg)
		vecChar.push_back(s.c_str());

	ParseParameters(vecChar.size(), &vecChar[0]);
}

TEST_CASE("boolarg")
{
	ResetArgs("-foo");
	REQUIRE(GetBoolArg("-foo", false));
	REQUIRE(GetBoolArg("-foo", true));

	REQUIRE(!GetBoolArg("-fo", false));
	REQUIRE(GetBoolArg("-fo", true));

	REQUIRE(!GetBoolArg("-fooo", false));
	REQUIRE(GetBoolArg("-fooo", true));

	ResetArgs("-foo=0");
	REQUIRE(!GetBoolArg("-foo", false));
	REQUIRE(!GetBoolArg("-foo", true));

	ResetArgs("-foo=1");
	REQUIRE(GetBoolArg("-foo", false));
	REQUIRE(GetBoolArg("-foo", true));

	// New 0.6 feature: auto-map -nosomething to !-something:
	ResetArgs("-nofoo");
	REQUIRE(!GetBoolArg("-foo", false));
	REQUIRE(!GetBoolArg("-foo", true));

	ResetArgs("-nofoo=1");
	REQUIRE(!GetBoolArg("-foo", false));
	REQUIRE(!GetBoolArg("-foo", true));

	ResetArgs("-foo -nofoo");  // -nofoo should win
	REQUIRE(!GetBoolArg("-foo", false));
	REQUIRE(!GetBoolArg("-foo", true));

	ResetArgs("-foo=1 -nofoo=1");  // -nofoo should win
	REQUIRE(!GetBoolArg("-foo", false));
	REQUIRE(!GetBoolArg("-foo", true));

	ResetArgs("-foo=0 -nofoo=0");  // -nofoo=0 should win
	REQUIRE(GetBoolArg("-foo", false));
	REQUIRE(GetBoolArg("-foo", true));

	// New 0.6 feature: treat -- same as -:
	ResetArgs("--foo=1");
	REQUIRE(GetBoolArg("-foo", false));
	REQUIRE(GetBoolArg("-foo", true));

	ResetArgs("--nofoo=1");
	REQUIRE(!GetBoolArg("-foo", false));
	REQUIRE(!GetBoolArg("-foo", true));

}

TEST_CASE("stringarg")
{
	ResetArgs("");
	REQUIRE(GetArg("-foo", "") == "");
	REQUIRE(GetArg("-foo", "eleven") == "eleven");

	ResetArgs("-foo -bar");
	REQUIRE(GetArg("-foo", "") == "");
	REQUIRE(GetArg("-foo", "eleven") == "");

	ResetArgs("-foo=");
	REQUIRE(GetArg("-foo", "") == "");
	REQUIRE(GetArg("-foo", "eleven") == "");

	ResetArgs("-foo=11");
	REQUIRE(GetArg("-foo", "") == "11");
	REQUIRE(GetArg("-foo", "eleven") == "11");

	ResetArgs("-foo=eleven");
	REQUIRE(GetArg("-foo", "") == "eleven");
	REQUIRE(GetArg("-foo", "eleven") == "eleven");

}

TEST_CASE("intarg")
{
	ResetArgs("");
	REQUIRE(GetArg("-foo", 11) == 11);
	REQUIRE(GetArg("-foo", 0) == 0);

	ResetArgs("-foo -bar");
	REQUIRE(GetArg("-foo", 11) == 0);
	REQUIRE(GetArg("-bar", 11) == 0);

	ResetArgs("-foo=11 -bar=12");
	REQUIRE(GetArg("-foo", 0) == 11);
	REQUIRE(GetArg("-bar", 11) == 12);

	ResetArgs("-foo=NaN -bar=NotANumber");
	REQUIRE(GetArg("-foo", 1) == 0);
	REQUIRE(GetArg("-bar", 11) == 0);
}

TEST_CASE("doubleulord")
{
	ResetArgs("--foo");
	REQUIRE(GetBoolArg("-foo", false) == true);

	ResetArgs("--foo=verbose --bar=1");
	REQUIRE(GetArg("-foo", "") == "verbose");
	REQUIRE(GetArg("-bar", 0) == 1);
}

TEST_CASE("boolargno")
{
	ResetArgs("-nofoo");
	REQUIRE(!GetBoolArg("-foo", true));
	REQUIRE(!GetBoolArg("-foo", false));

	ResetArgs("-nofoo=1");
	REQUIRE(!GetBoolArg("-foo", true));
	REQUIRE(!GetBoolArg("-foo", false));

	ResetArgs("-nofoo=0");
	REQUIRE(GetBoolArg("-foo", true));
	REQUIRE(GetBoolArg("-foo", false));

	ResetArgs("-foo --nofoo"); // --nofoo should win
	REQUIRE(!GetBoolArg("-foo", true));
	REQUIRE(!GetBoolArg("-foo", false));

	ResetArgs("-nofoo -foo"); // foo always wins:
	REQUIRE(GetBoolArg("-foo", true));
	REQUIRE(GetBoolArg("-foo", false));
}