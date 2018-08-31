// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//
// Unit tests for block-chain checkpoints
//

#include <catch2/catch.hpp>

#include "checkpoints.h"

#include "uint256.h"
#include "chainparams.h"

using namespace std;

TEST_CASE("sanity")
{
	const CCheckpointData& checkpoints = Params(CBaseChainParams::MAIN).Checkpoints();
	REQUIRE(Checkpoints::GetTotalBlocksEstimate(checkpoints) >= 0);
}
