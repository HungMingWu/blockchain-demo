// Copyright (c) 2013-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cstdio>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <catch2/catch.hpp>

#include "clientversion.h"
#include "consensus/validation.h"
#include "main.h" // For CheckBlock
#include "primitives/block.h"
#include "utiltime.h"


bool read_block(const std::string& filename, CBlock& block)
{
	namespace fs = boost::filesystem;
	fs::path testFile = fs::current_path() / "data" / filename;
#ifdef TEST_DATA_DIR
	if (!fs::exists(testFile))
	{
		testFile = fs::path(BOOST_PP_STRINGIZE(TEST_DATA_DIR)) / filename;
	}
#endif
	FILE* fp = fopen(testFile.string().c_str(), "rb");
	if (!fp) return false;

	fseek(fp, 8, SEEK_SET); // skip msgheader/size

	CAutoFile filein(fp, SER_DISK, CLIENT_VERSION);
	if (filein.IsNull()) return false;

	filein >> block;

	return true;
}

TEST_CASE("May15")
{
	// Putting a 1MB binary file in the git repository is not a great
	// idea, so this test is only run if you manually download
	// test/data/Mar12Fork.dat from
	// http://sourceforge.net/projects/bitcoin/files/Bitcoin/blockchain/Mar12Fork.dat/download
	unsigned int tMay15 = 1368576000;
	SetMockTime(tMay15); // Test as if it was right at May 15

	CBlock forkingBlock;
	if (read_block("Mar12Fork.dat", forkingBlock))
	{
		CValidationState state;

		// After May 15'th, big blocks are OK:
		forkingBlock.nTime = tMay15; // Invalidates PoW
		REQUIRE(CheckBlock(forkingBlock, state, false, false));
	}

	SetMockTime(0);
}