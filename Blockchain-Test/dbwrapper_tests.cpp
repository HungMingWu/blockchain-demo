// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <memory>
#include <boost/filesystem.hpp>
#include <catch2/catch.hpp>

#include "dbwrapper.h"
#include "uint256.h"
#include "random.h"


using namespace std;
using namespace boost::filesystem;

// Test if a string consists entirely of null characters
bool is_null_key(const vector<unsigned char>& key) {
	bool isnull = true;

	for (unsigned int i = 0; i < key.size(); i++)
		isnull &= (key[i] == '\x00');

	return isnull;
}

TEST_CASE("dbwrapper", "[dbwrapper]")
{
	// Perform tests both obfuscated and non-obfuscated.
	for (int i = 0; i < 2; i++) {
		bool obfuscate = (bool)i;
		path ph = temp_directory_path() / unique_path();
		CDBWrapper dbw(ph, (1 << 20), true, false, obfuscate);
		char key = 'k';
		uint256 in = GetRandHash();
		uint256 res;

		// Ensure that we're doing real obfuscation when obfuscate=true
		REQUIRE(obfuscate != is_null_key(dbw.GetObfuscateKey()));

		REQUIRE(dbw.Write(key, in));
		REQUIRE(dbw.Read(key, res));
		REQUIRE(res.ToString() == in.ToString());
	}
}

// Test batch operations
TEST_CASE("dbwrapper_batch", "[dbwrapper]")
{
	// Perform tests both obfuscated and non-obfuscated.
	for (int i = 0; i < 2; i++) {
		bool obfuscate = (bool)i;
		path ph = temp_directory_path() / unique_path();
		CDBWrapper dbw(ph, (1 << 20), true, false, obfuscate);

		char key = 'i';
		uint256 in = GetRandHash();
		char key2 = 'j';
		uint256 in2 = GetRandHash();
		char key3 = 'k';
		uint256 in3 = GetRandHash();

		uint256 res;
		CDBBatch batch(&dbw.GetObfuscateKey());

		batch.Write(key, in);
		batch.Write(key2, in2);
		batch.Write(key3, in3);

		// Remove key3 before it's even been written
		batch.Erase(key3);

		dbw.WriteBatch(batch);

		REQUIRE(dbw.Read(key, res));
		REQUIRE(res.ToString() == in.ToString());
		REQUIRE(dbw.Read(key2, res));
		REQUIRE(res.ToString() == in2.ToString());

		// key3 never should've been written
		REQUIRE(dbw.Read(key3, res) == false);
	}
}

TEST_CASE("dbwrapper_iterator", "[dbwrapper]")
{
	// Perform tests both obfuscated and non-obfuscated.
	for (int i = 0; i < 2; i++) {
		bool obfuscate = (bool)i;
		path ph = temp_directory_path() / unique_path();
		CDBWrapper dbw(ph, (1 << 20), true, false, obfuscate);

		// The two keys are intentionally chosen for ordering
		char key = 'j';
		uint256 in = GetRandHash();
		REQUIRE(dbw.Write(key, in));
		char key2 = 'k';
		uint256 in2 = GetRandHash();
		REQUIRE(dbw.Write(key2, in2));

		std::unique_ptr<CDBIterator> it(const_cast<CDBWrapper*>(&dbw)->NewIterator());

		// Be sure to seek past the obfuscation key (if it exists)
		it->Seek(key);

		char key_res;
		uint256 val_res;

		it->GetKey(key_res);
		it->GetValue(val_res);
		REQUIRE(key_res == key);
		REQUIRE(val_res.ToString() == in.ToString());

		it->Next();

		it->GetKey(key_res);
		it->GetValue(val_res);
		REQUIRE(key_res == key2);
		REQUIRE(val_res.ToString() == in2.ToString());

		it->Next();
		REQUIRE(it->Valid() == false);
	}
}

// Test that we do not obfuscation if there is existing data.
TEST_CASE("existing_data_no_obfuscate", "[dbwrapper]")
{
	// We're going to share this path between two wrappers
	path ph = temp_directory_path() / unique_path();
	create_directories(ph);

	// Set up a non-obfuscated wrapper to write some initial data.
	CDBWrapper* dbw = new CDBWrapper(ph, (1 << 10), false, false, false);
	char key = 'k';
	uint256 in = GetRandHash();
	uint256 res;

	REQUIRE(dbw->Write(key, in));
	REQUIRE(dbw->Read(key, res));
	REQUIRE(res.ToString() == in.ToString());

	// Call the destructor to free leveldb LOCK
	delete dbw;

	// Now, set up another wrapper that wants to obfuscate the same directory
	CDBWrapper odbw(ph, (1 << 10), false, false, true);

	// Check that the key/val we wrote with unobfuscated wrapper exists and 
	// is readable.
	uint256 res2;
	REQUIRE(odbw.Read(key, res2));
	REQUIRE(res2.ToString() == in.ToString());

	REQUIRE(!odbw.IsEmpty()); // There should be existing data
	REQUIRE(is_null_key(odbw.GetObfuscateKey())); // The key should be an empty string

	uint256 in2 = GetRandHash();
	uint256 res3;

	// Check that we can write successfully
	REQUIRE(odbw.Write(key, in2));
	REQUIRE(odbw.Read(key, res3));
	REQUIRE(res3.ToString() == in2.ToString());
}

// Ensure that we start obfuscating during a reindex.
TEST_CASE("existing_data_reindex", "[dbwrapper]")
{
	// We're going to share this path between two wrappers
	path ph = temp_directory_path() / unique_path();
	create_directories(ph);

	// Set up a non-obfuscated wrapper to write some initial data.
	CDBWrapper* dbw = new CDBWrapper(ph, (1 << 10), false, false, false);
	char key = 'k';
	uint256 in = GetRandHash();
	uint256 res;

	REQUIRE(dbw->Write(key, in));
	REQUIRE(dbw->Read(key, res));
	REQUIRE(res.ToString() == in.ToString());

	// Call the destructor to free leveldb LOCK
	delete dbw;

	// Simulate a -reindex by wiping the existing data store
	CDBWrapper odbw(ph, (1 << 10), false, true, true);

	// Check that the key/val we wrote with unobfuscated wrapper doesn't exist
	uint256 res2;
	REQUIRE(!odbw.Read(key, res2));
	REQUIRE(!is_null_key(odbw.GetObfuscateKey()));

	uint256 in2 = GetRandHash();
	uint256 res3;

	// Check that we can write successfully
	REQUIRE(odbw.Write(key, in2));
	REQUIRE(odbw.Read(key, res3));
	REQUIRE(res3.ToString() == in2.ToString());
}

TEST_CASE("iterator_ordering", "[dbwrapper]")
{
	path ph = temp_directory_path() / unique_path();
	CDBWrapper dbw(ph, (1 << 20), true, false, false);
	for (int x = 0x00; x < 256; ++x) {
		uint8_t key = x;
		uint32_t value = x * x;
		REQUIRE(dbw.Write(key, value));
	}

	std::unique_ptr<CDBIterator> it(const_cast<CDBWrapper*>(&dbw)->NewIterator());
	for (int c = 0; c < 2; ++c) {
		int seek_start;
		if (c == 0)
			seek_start = 0x00;
		else
			seek_start = 0x80;
		it->Seek((uint8_t)seek_start);
		for (int x = seek_start; x < 256; ++x) {
			uint8_t key;
			uint32_t value;
			REQUIRE(it->Valid());
			if (!it->Valid()) // Avoid spurious errors about invalid iterator's key and value in case of failure
				break;
			REQUIRE(it->GetKey(key));
			REQUIRE(it->GetValue(value));
			REQUIRE(key == x);
			REQUIRE(value == x * x);
			it->Next();
		}
		REQUIRE(!it->Valid());
	}
}

struct StringContentsSerializer {
	// Used to make two serialized objects the same while letting them have a different lengths
	// This is a terrible idea
	string str;
	StringContentsSerializer() {}
	StringContentsSerializer(const string& inp) : str(inp) {}

	StringContentsSerializer& operator+=(const string& s) {
		str += s;
		return *this;
	}
	StringContentsSerializer& operator+=(const StringContentsSerializer& s) { return *this += s.str; }

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
		if (ser_action.ForRead()) {
			str.clear();
			char c = 0;
			while (true) {
				try {
					READWRITE(c);
					str.push_back(c);
				}
				catch (const std::ios_base::failure& e) {
					break;
				}
			}
		}
		else {
			for (size_t i = 0; i < str.size(); i++)
				READWRITE(str[i]);
		}
	}
};

TEST_CASE("iterator_string_ordering", "[dbwrapper]")
{
	path ph = temp_directory_path() / unique_path();
	CDBWrapper dbw(ph, (1 << 20), true, false, false);
	for (int x = 0x00; x < 10; ++x) {
		for (int y = 0; y < 10; y++) {
			StringContentsSerializer key(std::to_string(x));
			for (int z = 0; z < y; z++)
				key += key;
			uint32_t value = x * x;
			REQUIRE(dbw.Write(key, value));
		}
	}

	std::unique_ptr<CDBIterator> it(const_cast<CDBWrapper*>(&dbw)->NewIterator());
	for (int c = 0; c < 2; ++c) {
		int seek_start;
		if (c == 0)
			seek_start = 0;
		else
			seek_start = 5;
		StringContentsSerializer seek_key(std::to_string(seek_start));
		it->Seek(seek_key);
		for (int x = seek_start; x < 10; ++x) {
			for (int y = 0; y < 10; y++) {
				string exp_key(std::to_string(x));
				for (int z = 0; z < y; z++)
					exp_key += exp_key;
				StringContentsSerializer key;
				uint32_t value;
				REQUIRE(it->Valid());
				if (!it->Valid()) // Avoid spurious errors about invalid iterator's key and value in case of failure
					break;
				REQUIRE(it->GetKey(key));
				REQUIRE(it->GetValue(value));
				REQUIRE(key.str == exp_key);
				REQUIRE(value == x * x);
				it->Next();
			}
		}
		REQUIRE(!it->Valid());
	}
}