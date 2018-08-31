// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <memenv.h>
#include <stdint.h>
#include <boost/filesystem.hpp>
#include <leveldb/cache.h>
#include <leveldb/env.h>
#include <leveldb/filter_policy.h>

#include "dbwrapper.h"
#include "random.h"
#include "util.h"
#include "utilstrencodings.h"
#include "Log.h"

void HandleError(const leveldb::Status& status)
{
	if (status.ok())
		return;
	LOG_ERROR("{}", status.ToString());
	if (status.IsCorruption())
		throw dbwrapper_error("Database corrupted");
	if (status.IsIOError())
		throw dbwrapper_error("Database I/O error");
	if (status.IsNotFound())
		throw dbwrapper_error("Database entry missing");
	throw dbwrapper_error("Unknown database error");
}

static leveldb::Options GetOptions(size_t nCacheSize)
{
	leveldb::Options options;
	options.block_cache = leveldb::NewLRUCache(nCacheSize / 2);
	options.write_buffer_size = nCacheSize / 4; // up to two write buffers may be held in memory simultaneously
	options.filter_policy = leveldb::NewBloomFilterPolicy(10);
	options.compression = leveldb::kNoCompression;
	options.max_open_files = 64;
	if (leveldb::kMajorVersion > 1 || (leveldb::kMajorVersion == 1 && leveldb::kMinorVersion >= 16)) {
		// LevelDB versions before 1.16 consider short writes to be corruption. Only trigger error
		// on corruption in later versions.
		options.paranoid_checks = true;
	}
	return options;
}

CDBWrapper::CDBWrapper(const boost::filesystem::path& path, size_t nCacheSize, bool fMemory, bool fWipe, bool obfuscate)
{
	readoptions.verify_checksums = true;
	iteroptions.verify_checksums = true;
	iteroptions.fill_cache = false;
	syncoptions.sync = true;
	options = GetOptions(nCacheSize);
	options.create_if_missing = true;
	if (fMemory) {
		penv.reset(leveldb::NewMemEnv(leveldb::Env::Default()));
		options.env = penv.get();
	}
	else {
		if (fWipe) {
			LOG_INFO("Wiping LevelDB in {}", path.string());
			leveldb::Status result = leveldb::DestroyDB(path.string(), options);
			HandleError(result);
		}
		TryCreateDirectory(path);
		LOG_INFO("Opening LevelDB in {}", path.string());
	}
	leveldb::DB *pDB = nullptr;
	leveldb::Status status = leveldb::DB::Open(options, path.string(), &pDB);
	HandleError(status);
	LOG_INFO("Opened LevelDB successfully");
	pdb.reset(pDB);

	// The base-case obfuscation key, which is a noop.
	obfuscate_key = std::vector<unsigned char>(OBFUSCATE_KEY_NUM_BYTES, '\000');

	bool key_exists = Read(OBFUSCATE_KEY_KEY, obfuscate_key);

	if (!key_exists && obfuscate && IsEmpty()) {
		// Initialize non-degenerate obfuscation if it won't upset
		// existing, non-obfuscated data.
		std::vector<unsigned char> new_key = CreateObfuscateKey();

		// Write `new_key` so we don't obfuscate the key with itself
		Write(OBFUSCATE_KEY_KEY, new_key);
		obfuscate_key = new_key;

		LOG_INFO("Wrote new obfuscate key for {}: {}", path.string(), GetObfuscateKeyHex());
	}

	LOG_INFO("Using obfuscation key for {}: {}", path.string(), GetObfuscateKeyHex());
}

CDBWrapper::~CDBWrapper()
{
	delete options.filter_policy;
	delete options.block_cache;
}

bool CDBWrapper::WriteBatch(CDBBatch& batch, bool fSync)
{
	leveldb::Status status = pdb->Write(fSync ? syncoptions : writeoptions, &batch.batch);
	HandleError(status);
	return true;
}

// Prefixed with null character to avoid collisions with other keys
//
// We must use a string constructor which specifies length so that we copy
// past the null-terminator.
const std::string CDBWrapper::OBFUSCATE_KEY_KEY("\000obfuscate_key", 14);

const unsigned int CDBWrapper::OBFUSCATE_KEY_NUM_BYTES = 8;

/**
 * Returns a string (consisting of 8 random bytes) suitable for use as an
 * obfuscating XOR key.
 */
std::vector<unsigned char> CDBWrapper::CreateObfuscateKey() const
{
	unsigned char buff[OBFUSCATE_KEY_NUM_BYTES];
	GetRandBytes(buff, OBFUSCATE_KEY_NUM_BYTES);
	return std::vector<unsigned char>(&buff[0], &buff[OBFUSCATE_KEY_NUM_BYTES]);

}

bool CDBWrapper::IsEmpty()
{
	std::unique_ptr<CDBIterator> it(NewIterator());
	it->SeekToFirst();
	return !(it->Valid());
}

const std::vector<unsigned char>& CDBWrapper::GetObfuscateKey() const
{
	return obfuscate_key;
}

std::string CDBWrapper::GetObfuscateKeyHex() const
{
	return HexStr(obfuscate_key);
}

CDBIterator::~CDBIterator() { delete piter; }
bool CDBIterator::Valid() { return piter->Valid(); }
void CDBIterator::SeekToFirst() { piter->SeekToFirst(); }
void CDBIterator::Next() { piter->Next(); }
