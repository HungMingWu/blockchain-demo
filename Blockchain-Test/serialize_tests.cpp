// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <catch2/catch.hpp>

#include "serialize.h"
#include "streams.h"
#include "hash.h"

#include <stdint.h>

using namespace std;

TEST_CASE("sizes")
{
	REQUIRE(sizeof(char) == GetSerializeSize(char(0), 0));
	REQUIRE(sizeof(int8_t) == GetSerializeSize(int8_t(0), 0));
	REQUIRE(sizeof(uint8_t) == GetSerializeSize(uint8_t(0), 0));
	REQUIRE(sizeof(int16_t) == GetSerializeSize(int16_t(0), 0));
	REQUIRE(sizeof(uint16_t) == GetSerializeSize(uint16_t(0), 0));
	REQUIRE(sizeof(int32_t) == GetSerializeSize(int32_t(0), 0));
	REQUIRE(sizeof(uint32_t) == GetSerializeSize(uint32_t(0), 0));
	REQUIRE(sizeof(int64_t) == GetSerializeSize(int64_t(0), 0));
	REQUIRE(sizeof(uint64_t) == GetSerializeSize(uint64_t(0), 0));
	REQUIRE(sizeof(float) == GetSerializeSize(float(0), 0));
	REQUIRE(sizeof(double) == GetSerializeSize(double(0), 0));
	// Bool is serialized as char
	REQUIRE(sizeof(char) == GetSerializeSize(bool(0), 0));

	// Sanity-check GetSerializeSize and c++ type matching
	REQUIRE(GetSerializeSize(char(0), 0) == 1);
	REQUIRE(GetSerializeSize(int8_t(0), 0) == 1);
	REQUIRE(GetSerializeSize(uint8_t(0), 0) == 1);
	REQUIRE(GetSerializeSize(int16_t(0), 0) == 2);
	REQUIRE(GetSerializeSize(uint16_t(0), 0) == 2);
	REQUIRE(GetSerializeSize(int32_t(0), 0) == 4);
	REQUIRE(GetSerializeSize(uint32_t(0), 0) == 4);
	REQUIRE(GetSerializeSize(int64_t(0), 0) == 8);
	REQUIRE(GetSerializeSize(uint64_t(0), 0) == 8);
	REQUIRE(GetSerializeSize(float(0), 0) == 4);
	REQUIRE(GetSerializeSize(double(0), 0) == 8);
	REQUIRE(GetSerializeSize(bool(0), 0) == 1);
}

TEST_CASE("floats_conversion")
{
	// Choose values that map unambigiously to binary floating point to avoid
	// rounding issues at the compiler side.
	REQUIRE(ser_uint32_to_float(0x00000000) == 0.0F);
	REQUIRE(ser_uint32_to_float(0x3f000000) == 0.5F);
	REQUIRE(ser_uint32_to_float(0x3f800000) == 1.0F);
	REQUIRE(ser_uint32_to_float(0x40000000) == 2.0F);
	REQUIRE(ser_uint32_to_float(0x40800000) == 4.0F);
	REQUIRE(ser_uint32_to_float(0x44444444) == 785.066650390625F);

	REQUIRE(ser_float_to_uint32(0.0F) == 0x00000000);
	REQUIRE(ser_float_to_uint32(0.5F) == 0x3f000000);
	REQUIRE(ser_float_to_uint32(1.0F) == 0x3f800000);
	REQUIRE(ser_float_to_uint32(2.0F) == 0x40000000);
	REQUIRE(ser_float_to_uint32(4.0F) == 0x40800000);
	REQUIRE(ser_float_to_uint32(785.066650390625F) == 0x44444444);
}

TEST_CASE("doubles_conversion")
{
	// Choose values that map unambigiously to binary floating point to avoid
	// rounding issues at the compiler side.
	REQUIRE(ser_uint64_to_double(0x0000000000000000ULL) == 0.0);
	REQUIRE(ser_uint64_to_double(0x3fe0000000000000ULL) == 0.5);
	REQUIRE(ser_uint64_to_double(0x3ff0000000000000ULL) == 1.0);
	REQUIRE(ser_uint64_to_double(0x4000000000000000ULL) == 2.0);
	REQUIRE(ser_uint64_to_double(0x4010000000000000ULL) == 4.0);
	REQUIRE(ser_uint64_to_double(0x4088888880000000ULL) == 785.066650390625);

	REQUIRE(ser_double_to_uint64(0.0) == 0x0000000000000000ULL);
	REQUIRE(ser_double_to_uint64(0.5) == 0x3fe0000000000000ULL);
	REQUIRE(ser_double_to_uint64(1.0) == 0x3ff0000000000000ULL);
	REQUIRE(ser_double_to_uint64(2.0) == 0x4000000000000000ULL);
	REQUIRE(ser_double_to_uint64(4.0) == 0x4010000000000000ULL);
	REQUIRE(ser_double_to_uint64(785.066650390625) == 0x4088888880000000ULL);
}
/*
Python code to generate the below hashes:

	def reversed_hex(x):
		return binascii.hexlify(''.join(reversed(x)))
	def dsha256(x):
		return hashlib.sha256(hashlib.sha256(x).digest()).digest()

	reversed_hex(dsha256(''.join(struct.pack('<f', x) for x in range(0,1000)))) == '8e8b4cf3e4df8b332057e3e23af42ebc663b61e0495d5e7e32d85099d7f3fe0c'
	reversed_hex(dsha256(''.join(struct.pack('<d', x) for x in range(0,1000)))) == '43d0c82591953c4eafe114590d392676a01585d25b25d433557f0d7878b23f96'
*/
TEST_CASE("floats")
{
	CDataStream ss(SER_DISK, 0);
	// encode
	for (int i = 0; i < 1000; i++) {
		ss << float(i);
	}
	REQUIRE(Hash(ss.begin(), ss.end()) == uint256S("8e8b4cf3e4df8b332057e3e23af42ebc663b61e0495d5e7e32d85099d7f3fe0c"));

	// decode
	for (int i = 0; i < 1000; i++) {
		float j;
		ss >> j;
		REQUIRE(i == j);
	}
}

TEST_CASE("doubles")
{
	CDataStream ss(SER_DISK, 0);
	// encode
	for (int i = 0; i < 1000; i++) {
		ss << double(i);
	}
	REQUIRE(Hash(ss.begin(), ss.end()) == uint256S("43d0c82591953c4eafe114590d392676a01585d25b25d433557f0d7878b23f96"));

	// decode
	for (int i = 0; i < 1000; i++) {
		double j;
		ss >> j;
		REQUIRE(i == j);
	}
}

TEST_CASE("varints")
{
	// encode

	CDataStream ss(SER_DISK, 0);
	CDataStream::size_type size = 0;
	for (int i = 0; i < 100000; i++) {
		ss << VARINT(i);
		size += ::GetSerializeSize(VARINT(i), 0, 0);
		REQUIRE(size == ss.size());
	}

	for (uint64_t i = 0; i < 100000000000ULL; i += 999999937) {
		ss << VARINT(i);
		size += ::GetSerializeSize(VARINT(i), 0, 0);
		REQUIRE(size == ss.size());
	}

	// decode
	for (int i = 0; i < 100000; i++) {
		int j = -1;
		ss >> VARINT(j);
		REQUIRE(i == j);
	}

	for (uint64_t i = 0; i < 100000000000ULL; i += 999999937) {
		uint64_t j = -1;
		ss >> VARINT(j);
		REQUIRE(i == j);
	}
}

TEST_CASE("compactsize")
{
	CDataStream ss(SER_DISK, 0);
	vector<char>::size_type i, j;

	for (i = 1; i <= MAX_SIZE; i *= 2)
	{
		WriteCompactSize(ss, i - 1);
		WriteCompactSize(ss, i);
	}
	for (i = 1; i <= MAX_SIZE; i *= 2)
	{
		j = ReadCompactSize(ss);
		REQUIRE((i - 1) == j);
		j = ReadCompactSize(ss);
		REQUIRE(i == j);
	}
}

static bool isCanonicalException(const std::ios_base::failure& ex)
{
	std::ios_base::failure expectedException("non-canonical ReadCompactSize()");

	// The string returned by what() can be different for different platforms.
	// Instead of directly comparing the ex.what() with an expected string,
	// create an instance of exception to see if ex.what() matches 
	// the expected explanatory string returned by the exception instance. 
	return strcmp(expectedException.what(), ex.what()) == 0;
}

#if 0
TEST_CASE("noncanonical")
{
	// Write some non-canonical CompactSize encodings, and
	// make sure an exception is thrown when read back.
	CDataStream ss(SER_DISK, 0);
	vector<char>::size_type n;

	// zero encoded with three bytes:
	ss.write("\xfd\x00\x00", 3);
	BOOST_CHECK_EXCEPTION(ReadCompactSize(ss), std::ios_base::failure, isCanonicalException);

	// 0xfc encoded with three bytes:
	ss.write("\xfd\xfc\x00", 3);
	BOOST_CHECK_EXCEPTION(ReadCompactSize(ss), std::ios_base::failure, isCanonicalException);

	// 0xfd encoded with three bytes is OK:
	ss.write("\xfd\xfd\x00", 3);
	n = ReadCompactSize(ss);
	BOOST_CHECK(n == 0xfd);

	// zero encoded with five bytes:
	ss.write("\xfe\x00\x00\x00\x00", 5);
	BOOST_CHECK_EXCEPTION(ReadCompactSize(ss), std::ios_base::failure, isCanonicalException);

	// 0xffff encoded with five bytes:
	ss.write("\xfe\xff\xff\x00\x00", 5);
	BOOST_CHECK_EXCEPTION(ReadCompactSize(ss), std::ios_base::failure, isCanonicalException);

	// zero encoded with nine bytes:
	ss.write("\xff\x00\x00\x00\x00\x00\x00\x00\x00", 9);
	BOOST_CHECK_EXCEPTION(ReadCompactSize(ss), std::ios_base::failure, isCanonicalException);

	// 0x01ffffff encoded with nine bytes:
	ss.write("\xff\xff\xff\xff\x01\x00\x00\x00\x00", 9);
	BOOST_CHECK_EXCEPTION(ReadCompactSize(ss), std::ios_base::failure, isCanonicalException);
}
#endif

TEST_CASE("insert_delete")
{
	// Test inserting/deleting bytes.
	CDataStream ss(SER_DISK, 0);
	REQUIRE(ss.size() == 0);

	ss.write("\x00\x01\x02\xff", 4);
	REQUIRE(ss.size() == 4);

	char c = (char)11;

	// Inserting at beginning/end/middle:
	ss.insert(ss.begin(), c);
	REQUIRE(ss.size() == 5);
	REQUIRE(ss[0] == c);
	REQUIRE(ss[1] == 0);

	ss.insert(ss.end(), c);
	REQUIRE(ss.size() == 6);
	REQUIRE(ss[4] == (char)0xff);
	REQUIRE(ss[5] == c);

	ss.insert(ss.begin() + 2, c);
	REQUIRE(ss.size() == 7);
	REQUIRE(ss[2] == c);

	// Delete at beginning/end/middle
	ss.erase(ss.begin());
	REQUIRE(ss.size() == 6);
	REQUIRE(ss[0] == 0);

	ss.erase(ss.begin() + ss.size() - 1);
	REQUIRE(ss.size() == 5);
	REQUIRE(ss[4] == (char)0xff);

	ss.erase(ss.begin() + 1);
	REQUIRE(ss.size() == 4);
	REQUIRE(ss[0] == 0);
	REQUIRE(ss[1] == 1);
	REQUIRE(ss[2] == 2);
	REQUIRE(ss[3] == (char)0xff);

	// Make sure GetAndClear does the right thing:
	CSerializeData d;
	ss.GetAndClear(d);
	REQUIRE(ss.size() == 0);
}