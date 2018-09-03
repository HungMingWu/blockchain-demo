// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <catch2/catch.hpp>

#include "util.h"

#include "clientversion.h"
#include "primitives/transaction.h"
#include "random.h"
#include "sync.h"
#include "utilstrencodings.h"
#include "utilmoneystr.h"

#include <stdint.h>
#include <vector>



using namespace std;

TEST_CASE("util_criticalsection")
{
	CCriticalSection cs;

	do {
		LOCK(cs);
		break;

		INFO("break was swallowed!");
	} while (0);

	do {
		TRY_LOCK(cs, lockTest);
		if (lockTest)
			break;

		INFO("break was swallowed!");
	} while (0);
}

static const unsigned char ParseHex_expected[65] = {
	0x04, 0x67, 0x8a, 0xfd, 0xb0, 0xfe, 0x55, 0x48, 0x27, 0x19, 0x67, 0xf1, 0xa6, 0x71, 0x30, 0xb7,
	0x10, 0x5c, 0xd6, 0xa8, 0x28, 0xe0, 0x39, 0x09, 0xa6, 0x79, 0x62, 0xe0, 0xea, 0x1f, 0x61, 0xde,
	0xb6, 0x49, 0xf6, 0xbc, 0x3f, 0x4c, 0xef, 0x38, 0xc4, 0xf3, 0x55, 0x04, 0xe5, 0x1e, 0xc1, 0x12,
	0xde, 0x5c, 0x38, 0x4d, 0xf7, 0xba, 0x0b, 0x8d, 0x57, 0x8a, 0x4c, 0x70, 0x2b, 0x6b, 0xf1, 0x1d,
	0x5f
};

TEST_CASE("util_ParseHex")
{
	std::vector<unsigned char> result;
	std::vector<unsigned char> expected(ParseHex_expected, ParseHex_expected + sizeof(ParseHex_expected));
	// Basic test vector
	result = ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f");
	REQUIRE(result == expected);

	// Spaces between bytes must be supported
	result = ParseHex("12 34 56 78");
	REQUIRE((result.size() == 4 && result[0] == 0x12 && result[1] == 0x34 && result[2] == 0x56 && result[3] == 0x78));

	// Stop parsing at invalid value
	result = ParseHex("1234 invalid 1234");
	REQUIRE((result.size() == 2 && result[0] == 0x12 && result[1] == 0x34));
}

TEST_CASE("util_HexStr")
{
	REQUIRE(
		HexStr(ParseHex_expected, ParseHex_expected + sizeof(ParseHex_expected)) ==
		"04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f");

	REQUIRE(
		HexStr(ParseHex_expected, ParseHex_expected + 5, true) ==
		"04 67 8a fd b0");

	REQUIRE(
		HexStr(ParseHex_expected, ParseHex_expected, true) ==
		"");

	std::vector<unsigned char> ParseHex_vec(ParseHex_expected, ParseHex_expected + 5);

	REQUIRE(
		HexStr(ParseHex_vec, true) ==
		"04 67 8a fd b0");
}


TEST_CASE("util_DateTimeStrFormat")
{
	REQUIRE(DateTimeStrFormat("%Y-%m-%d %H:%M:%S", 0) == "1970-01-01 00:00:00");
	REQUIRE(DateTimeStrFormat("%Y-%m-%d %H:%M:%S", 0x7FFFFFFF) == "2038-01-19 03:14:07");
	REQUIRE(DateTimeStrFormat("%Y-%m-%d %H:%M:%S", 1317425777) == "2011-09-30 23:36:17");
	REQUIRE(DateTimeStrFormat("%Y-%m-%d %H:%M", 1317425777) == "2011-09-30 23:36");
	REQUIRE(DateTimeStrFormat("%a, %d %b %Y %H:%M:%S +0000", 1317425777) == "Fri, 30 Sep 2011 23:36:17 +0000");
}

TEST_CASE("util_ParseParameters")
{
	const char *argv_test[] = { "-ignored", "-a", "-b", "-ccc=argument", "-ccc=multiple", "f", "-d=e" };

	ParseParameters(0, (char**)argv_test);
	REQUIRE((mapArgs.empty() && mapMultiArgs.empty()));

	ParseParameters(1, (char**)argv_test);
	REQUIRE((mapArgs.empty() && mapMultiArgs.empty()));

	ParseParameters(5, (char**)argv_test);
	// expectation: -ignored is ignored (program name argument),
	// -a, -b and -ccc end up in map, -d ignored because it is after
	// a non-option argument (non-GNU option parsing)
	REQUIRE((mapArgs.size() == 3 && mapMultiArgs.size() == 3));
	REQUIRE((mapArgs.count("-a") && mapArgs.count("-b") && mapArgs.count("-ccc")
		&& !mapArgs.count("f") && !mapArgs.count("-d")));
	REQUIRE((mapMultiArgs.count("-a") && mapMultiArgs.count("-b") && mapMultiArgs.count("-ccc")
		&& !mapMultiArgs.count("f") && !mapMultiArgs.count("-d")));

	REQUIRE((mapArgs["-a"] == "" && mapArgs["-ccc"] == "multiple"));
	REQUIRE(mapMultiArgs["-ccc"].size() == 2);
}

TEST_CASE("util_GetArg")
{
	mapArgs.clear();
	mapArgs["strtest1"] = "string...";
	// strtest2 undefined on purpose
	mapArgs["inttest1"] = "12345";
	mapArgs["inttest2"] = "81985529216486895";
	// inttest3 undefined on purpose
	mapArgs["booltest1"] = "";
	// booltest2 undefined on purpose
	mapArgs["booltest3"] = "0";
	mapArgs["booltest4"] = "1";

	REQUIRE(GetArg("strtest1", "default") == "string...");
	REQUIRE(GetArg("strtest2", "default") == "default");
	REQUIRE(GetArg("inttest1", -1) == 12345);
	REQUIRE(GetArg("inttest2", -1) == 81985529216486895LL);
	REQUIRE(GetArg("inttest3", -1) == -1);
	REQUIRE(GetBoolArg("booltest1", false) == true);
	REQUIRE(GetBoolArg("booltest2", false) == false);
	REQUIRE(GetBoolArg("booltest3", false) == false);
	REQUIRE(GetBoolArg("booltest4", false) == true);
}

TEST_CASE("util_FormatMoney")
{
	REQUIRE(FormatMoney(0) == "0.00");
	REQUIRE(FormatMoney((COIN / 10000) * 123456789) == "12345.6789");
	REQUIRE(FormatMoney(-COIN) == "-1.00");

	REQUIRE(FormatMoney(COIN * 100000000) == "100000000.00");
	REQUIRE(FormatMoney(COIN * 10000000) == "10000000.00");
	REQUIRE(FormatMoney(COIN * 1000000) == "1000000.00");
	REQUIRE(FormatMoney(COIN * 100000) == "100000.00");
	REQUIRE(FormatMoney(COIN * 10000) == "10000.00");
	REQUIRE(FormatMoney(COIN * 1000) == "1000.00");
	REQUIRE(FormatMoney(COIN * 100) == "100.00");
	REQUIRE(FormatMoney(COIN * 10) == "10.00");
	REQUIRE(FormatMoney(COIN) == "1.00");
	REQUIRE(FormatMoney(COIN / 10) == "0.10");
	REQUIRE(FormatMoney(COIN / 100) == "0.01");
	REQUIRE(FormatMoney(COIN / 1000) == "0.001");
	REQUIRE(FormatMoney(COIN / 10000) == "0.0001");
	REQUIRE(FormatMoney(COIN / 100000) == "0.00001");
	REQUIRE(FormatMoney(COIN / 1000000) == "0.000001");
	REQUIRE(FormatMoney(COIN / 10000000) == "0.0000001");
	REQUIRE(FormatMoney(COIN / 100000000) == "0.00000001");
}

TEST_CASE("util_ParseMoney")
{
	CAmount ret = 0;
	REQUIRE(ParseMoney("0.0", ret));
	REQUIRE(ret == 0);

	REQUIRE(ParseMoney("12345.6789", ret));
	REQUIRE(ret == (COIN / 10000) * 123456789);

	REQUIRE(ParseMoney("100000000.00", ret));
	REQUIRE(ret == COIN * 100000000);
	REQUIRE(ParseMoney("10000000.00", ret));
	REQUIRE(ret == COIN * 10000000);
	REQUIRE(ParseMoney("1000000.00", ret));
	REQUIRE(ret == COIN * 1000000);
	REQUIRE(ParseMoney("100000.00", ret));
	REQUIRE(ret == COIN * 100000);
	REQUIRE(ParseMoney("10000.00", ret));
	REQUIRE(ret == COIN * 10000);
	REQUIRE(ParseMoney("1000.00", ret));
	REQUIRE(ret == COIN * 1000);
	REQUIRE(ParseMoney("100.00", ret));
	REQUIRE(ret == COIN * 100);
	REQUIRE(ParseMoney("10.00", ret));
	REQUIRE(ret == COIN * 10);
	REQUIRE(ParseMoney("1.00", ret));
	REQUIRE(ret == COIN);
	REQUIRE(ParseMoney("1", ret));
	REQUIRE(ret == COIN);
	REQUIRE(ParseMoney("0.1", ret));
	REQUIRE(ret == COIN / 10);
	REQUIRE(ParseMoney("0.01", ret));
	REQUIRE(ret == COIN / 100);
	REQUIRE(ParseMoney("0.001", ret));
	REQUIRE(ret == COIN / 1000);
	REQUIRE(ParseMoney("0.0001", ret));
	REQUIRE(ret == COIN / 10000);
	REQUIRE(ParseMoney("0.00001", ret));
	REQUIRE(ret == COIN / 100000);
	REQUIRE(ParseMoney("0.000001", ret));
	REQUIRE(ret == COIN / 1000000);
	REQUIRE(ParseMoney("0.0000001", ret));
	REQUIRE(ret == COIN / 10000000);
	REQUIRE(ParseMoney("0.00000001", ret));
	REQUIRE(ret == COIN / 100000000);

	// Attempted 63 bit overflow should fail
	REQUIRE(!ParseMoney("92233720368.54775808", ret));

	// Parsing negative amounts must fail
	REQUIRE(!ParseMoney("-1", ret));
}

TEST_CASE("util_IsHex")
{
	REQUIRE(IsHex("00"));
	REQUIRE(IsHex("00112233445566778899aabbccddeeffAABBCCDDEEFF"));
	REQUIRE(IsHex("ff"));
	REQUIRE(IsHex("FF"));

	REQUIRE(!IsHex(""));
	REQUIRE(!IsHex("0"));
	REQUIRE(!IsHex("a"));
	REQUIRE(!IsHex("eleven"));
	REQUIRE(!IsHex("00xx00"));
	REQUIRE(!IsHex("0x0000"));
}

TEST_CASE("util_seed_insecure_rand")
{
	int i;
	int count = 0;

	seed_insecure_rand(true);

	for (int mod = 2; mod < 11; mod++)
	{
		int mask = 1;
		// Really rough binomal confidence approximation.
		int err = 30 * 10000. / mod * sqrt((1. / mod * (1 - 1. / mod)) / 10000.);
		//mask is 2^ceil(log2(mod))-1
		while (mask < mod - 1)mask = (mask << 1) + 1;

		count = 0;
		//How often does it get a zero from the uniform range [0,mod)?
		for (i = 0; i < 10000; i++)
		{
			uint32_t rval;
			do {
				rval = insecure_rand()&mask;
			} while (rval >= (uint32_t)mod);
			count += rval == 0;
		}
		REQUIRE(count <= 10000 / mod + err);
		REQUIRE(count >= 10000 / mod - err);
	}
}

TEST_CASE("util_TimingResistantEqual")
{
	REQUIRE(TimingResistantEqual(std::string(""), std::string("")));
	REQUIRE(!TimingResistantEqual(std::string("abc"), std::string("")));
	REQUIRE(!TimingResistantEqual(std::string(""), std::string("abc")));
	REQUIRE(!TimingResistantEqual(std::string("a"), std::string("aa")));
	REQUIRE(!TimingResistantEqual(std::string("aa"), std::string("a")));
	REQUIRE(TimingResistantEqual(std::string("abc"), std::string("abc")));
	REQUIRE(!TimingResistantEqual(std::string("abc"), std::string("aba")));
}

/* Check for mingw/wine issue #3494
 * Remove this test before time.ctime(0xffffffff) == 'Sun Feb  7 07:28:15 2106'
 */
TEST_CASE("gettime")
{
	REQUIRE((GetTime() & ~0xFFFFFFFFLL) == 0);
}

TEST_CASE("test_ParseInt32")
{
	int32_t n;
	// Valid values
	REQUIRE(ParseInt32("1234", NULL));
	REQUIRE((ParseInt32("0", &n) && n == 0));
	REQUIRE((ParseInt32("1234", &n) && n == 1234));
	REQUIRE((ParseInt32("01234", &n) && n == 1234)); // no octal
	REQUIRE((ParseInt32("2147483647", &n) && n == 2147483647));
	REQUIRE((ParseInt32("-2147483648", &n) && n == -2147483648));
	REQUIRE((ParseInt32("-1234", &n) && n == -1234));
	// Invalid values
	REQUIRE(!ParseInt32("", &n));
	REQUIRE(!ParseInt32(" 1", &n)); // no padding inside
	REQUIRE(!ParseInt32("1 ", &n));
	REQUIRE(!ParseInt32("1a", &n));
	REQUIRE(!ParseInt32("aap", &n));
	REQUIRE(!ParseInt32("0x1", &n)); // no hex
	REQUIRE(!ParseInt32("0x1", &n)); // no hex
	const char test_bytes[] = { '1', 0, '1' };
	std::string teststr(test_bytes, sizeof(test_bytes));
	REQUIRE(!ParseInt32(teststr, &n)); // no embedded NULs
	// Overflow and underflow
	REQUIRE(!ParseInt32("-2147483649", NULL));
	REQUIRE(!ParseInt32("2147483648", NULL));
	REQUIRE(!ParseInt32("-32482348723847471234", NULL));
	REQUIRE(!ParseInt32("32482348723847471234", NULL));
}

TEST_CASE("test_ParseInt64")
{
	int64_t n;
	// Valid values
	REQUIRE(ParseInt64("1234", NULL));
	REQUIRE((ParseInt64("0", &n) && n == 0LL));
	REQUIRE((ParseInt64("1234", &n) && n == 1234LL));
	REQUIRE((ParseInt64("01234", &n) && n == 1234LL)); // no octal
	REQUIRE((ParseInt64("2147483647", &n) && n == 2147483647LL));
	REQUIRE((ParseInt64("-2147483648", &n) && n == -2147483648LL));
	REQUIRE((ParseInt64("9223372036854775807", &n) && n == (int64_t)9223372036854775807));
	REQUIRE((ParseInt64("-9223372036854775808", &n) && n == (int64_t)-9223372036854775807 - 1));
	REQUIRE((ParseInt64("-1234", &n) && n == -1234LL));
	// Invalid values
	REQUIRE(!ParseInt64("", &n));
	REQUIRE(!ParseInt64(" 1", &n)); // no padding inside
	REQUIRE(!ParseInt64("1 ", &n));
	REQUIRE(!ParseInt64("1a", &n));
	REQUIRE(!ParseInt64("aap", &n));
	REQUIRE(!ParseInt64("0x1", &n)); // no hex
	const char test_bytes[] = { '1', 0, '1' };
	std::string teststr(test_bytes, sizeof(test_bytes));
	REQUIRE(!ParseInt64(teststr, &n)); // no embedded NULs
	// Overflow and underflow
	REQUIRE(!ParseInt64("-9223372036854775809", NULL));
	REQUIRE(!ParseInt64("9223372036854775808", NULL));
	REQUIRE(!ParseInt64("-32482348723847471234", NULL));
	REQUIRE(!ParseInt64("32482348723847471234", NULL));
}

TEST_CASE("test_ParseDouble")
{
	double n;
	// Valid values
	REQUIRE(ParseDouble("1234", NULL));
	REQUIRE((ParseDouble("0", &n) && n == 0.0));
	REQUIRE((ParseDouble("1234", &n) && n == 1234.0));
	REQUIRE((ParseDouble("01234", &n) && n == 1234.0)); // no octal
	REQUIRE((ParseDouble("2147483647", &n) && n == 2147483647.0));
	REQUIRE((ParseDouble("-2147483648", &n) && n == -2147483648.0));
	REQUIRE((ParseDouble("-1234", &n) && n == -1234.0));
	REQUIRE((ParseDouble("1e6", &n) && n == 1e6));
	REQUIRE((ParseDouble("-1e6", &n) && n == -1e6));
	// Invalid values
	REQUIRE(!ParseDouble("", &n));
	REQUIRE(!ParseDouble(" 1", &n)); // no padding inside
	REQUIRE(!ParseDouble("1 ", &n));
	REQUIRE(!ParseDouble("1a", &n));
	REQUIRE(!ParseDouble("aap", &n));
	REQUIRE(!ParseDouble("0x1", &n)); // no hex
	const char test_bytes[] = { '1', 0, '1' };
	std::string teststr(test_bytes, sizeof(test_bytes));
	REQUIRE(!ParseDouble(teststr, &n)); // no embedded NULs
	// Overflow and underflow
	REQUIRE(!ParseDouble("-1e10000", NULL));
	REQUIRE(!ParseDouble("1e10000", NULL));
}

TEST_CASE("test_FormatParagraph")
{
	REQUIRE(FormatParagraph("", 79, 0) == "");
	REQUIRE(FormatParagraph("test", 79, 0) == "test");
	REQUIRE(FormatParagraph(" test", 79, 0) == "test");
	REQUIRE(FormatParagraph("test test", 79, 0) == "test test");
	REQUIRE(FormatParagraph("test test", 4, 0) == "test\ntest");
	REQUIRE(FormatParagraph("testerde test ", 4, 0) == "testerde\ntest");
	REQUIRE(FormatParagraph("test test", 4, 4) == "test\n    test");
	REQUIRE(FormatParagraph("This is a very long test string. This is a second sentence in the very long test string.") == "This is a very long test string. This is a second sentence in the very long\ntest string.");
}

TEST_CASE("test_FormatSubVersion")
{
	std::vector<std::string> comments;
	comments.push_back(std::string("comment1"));
	std::vector<std::string> comments2;
	comments2.push_back(std::string("comment1"));
	comments2.push_back(SanitizeString(std::string("Comment2; .,_?@-; !\"#$%&'()*+/<=>[]\\^`{|}~"), SAFE_CHARS_UA_COMMENT)); // Semicolon is discouraged but not forbidden by BIP-0014
	REQUIRE(FormatSubVersion("Test", 99900, std::vector<std::string>()) == std::string("/Test:0.9.99/"));
	REQUIRE(FormatSubVersion("Test", 99900, comments) == std::string("/Test:0.9.99(comment1)/"));
	REQUIRE(FormatSubVersion("Test", 99900, comments2) == std::string("/Test:0.9.99(comment1; Comment2; .,_?@-; )/"));
}

TEST_CASE("test_ParseFixedPoint")
{
	int64_t amount = 0;
	REQUIRE(ParseFixedPoint("0", 8, &amount));
	REQUIRE(amount == 0LL);
	REQUIRE(ParseFixedPoint("1", 8, &amount));
	REQUIRE(amount == 100000000LL);
	REQUIRE(ParseFixedPoint("0.0", 8, &amount));
	REQUIRE(amount == 0LL);
	REQUIRE(ParseFixedPoint("-0.1", 8, &amount));
	REQUIRE(amount == -10000000LL);
	REQUIRE(ParseFixedPoint("1.1", 8, &amount));
	REQUIRE(amount == 110000000LL);
	REQUIRE(ParseFixedPoint("1.10000000000000000", 8, &amount));
	REQUIRE(amount == 110000000LL);
	REQUIRE(ParseFixedPoint("1.1e1", 8, &amount));
	REQUIRE(amount == 1100000000LL);
	REQUIRE(ParseFixedPoint("1.1e-1", 8, &amount));
	REQUIRE(amount == 11000000LL);
	REQUIRE(ParseFixedPoint("1000", 8, &amount));
	REQUIRE(amount == 100000000000LL);
	REQUIRE(ParseFixedPoint("-1000", 8, &amount));
	REQUIRE(amount == -100000000000LL);
	REQUIRE(ParseFixedPoint("0.00000001", 8, &amount));
	REQUIRE(amount == 1LL);
	REQUIRE(ParseFixedPoint("0.0000000100000000", 8, &amount));
	REQUIRE(amount == 1LL);
	REQUIRE(ParseFixedPoint("-0.00000001", 8, &amount));
	REQUIRE(amount == -1LL);
	REQUIRE(ParseFixedPoint("1000000000.00000001", 8, &amount));
	REQUIRE(amount == 100000000000000001LL);
	REQUIRE(ParseFixedPoint("9999999999.99999999", 8, &amount));
	REQUIRE(amount == 999999999999999999LL);
	REQUIRE(ParseFixedPoint("-9999999999.99999999", 8, &amount));
	REQUIRE(amount == -999999999999999999LL);

	REQUIRE(!ParseFixedPoint("", 8, &amount));
	REQUIRE(!ParseFixedPoint("-", 8, &amount));
	REQUIRE(!ParseFixedPoint("a-1000", 8, &amount));
	REQUIRE(!ParseFixedPoint("-a1000", 8, &amount));
	REQUIRE(!ParseFixedPoint("-1000a", 8, &amount));
	REQUIRE(!ParseFixedPoint("-01000", 8, &amount));
	REQUIRE(!ParseFixedPoint("00.1", 8, &amount));
	REQUIRE(!ParseFixedPoint(".1", 8, &amount));
	REQUIRE(!ParseFixedPoint("--0.1", 8, &amount));
	REQUIRE(!ParseFixedPoint("0.000000001", 8, &amount));
	REQUIRE(!ParseFixedPoint("-0.000000001", 8, &amount));
	REQUIRE(!ParseFixedPoint("0.00000001000000001", 8, &amount));
	REQUIRE(!ParseFixedPoint("-10000000000.00000000", 8, &amount));
	REQUIRE(!ParseFixedPoint("10000000000.00000000", 8, &amount));
	REQUIRE(!ParseFixedPoint("-10000000000.00000001", 8, &amount));
	REQUIRE(!ParseFixedPoint("10000000000.00000001", 8, &amount));
	REQUIRE(!ParseFixedPoint("-10000000000.00000009", 8, &amount));
	REQUIRE(!ParseFixedPoint("10000000000.00000009", 8, &amount));
	REQUIRE(!ParseFixedPoint("-99999999999.99999999", 8, &amount));
	REQUIRE(!ParseFixedPoint("99999909999.09999999", 8, &amount));
	REQUIRE(!ParseFixedPoint("92233720368.54775807", 8, &amount));
	REQUIRE(!ParseFixedPoint("92233720368.54775808", 8, &amount));
	REQUIRE(!ParseFixedPoint("-92233720368.54775808", 8, &amount));
	REQUIRE(!ParseFixedPoint("-92233720368.54775809", 8, &amount));
	REQUIRE(!ParseFixedPoint("1.1e", 8, &amount));
	REQUIRE(!ParseFixedPoint("1.1e-", 8, &amount));
	REQUIRE(!ParseFixedPoint("1.", 8, &amount));
}