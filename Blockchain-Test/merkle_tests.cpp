// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <tuple>
#include <catch2/catch.hpp>

#include "consensus/merkle.h"
#include "random.h"
#include "hash.h"

// Older version of the merkle root computation code, for comparison.
static std::tuple<uint256, bool, std::vector<uint256>> BlockBuildMerkleTree(const CBlock& block)
{
	std::vector<uint256> vMerkleTree;
	for (const auto & vtx : block.vtx)
		vMerkleTree.push_back(vtx.GetHash());
	int j = 0;
	bool mutated = false;
	for (int nSize = block.vtx.size(); nSize > 1; nSize = (nSize + 1) / 2)
	{
		for (int i = 0; i < nSize; i += 2)
		{
			int i2 = std::min(i + 1, nSize - 1);
			if (i2 == i + 1 && i2 + 1 == nSize && vMerkleTree[j + i] == vMerkleTree[j + i2]) {
				// Two identical hashes at the end of the list at a particular level.
				mutated = true;
			}
			vMerkleTree.push_back(Hash(vMerkleTree[j + i].begin(), vMerkleTree[j + i].end(),
				vMerkleTree[j + i2].begin(), vMerkleTree[j + i2].end()));
		}
		j += nSize;
	}
	uint256 value = vMerkleTree.empty() ? uint256() : vMerkleTree.back();
	return { value, mutated, vMerkleTree  };
}

// Older version of the merkle branch computation code, for comparison.
static std::vector<uint256> BlockGetMerkleBranch(const CBlock& block, const std::vector<uint256>& vMerkleTree, size_t nIndex)
{
	std::vector<uint256> vMerkleBranch;
	for (size_t nSize = block.vtx.size(), j = 0; nSize > 1; j += nSize, nSize = (nSize + 1) / 2)
	{
		size_t i = std::min(nIndex ^ 1, nSize - 1);
		vMerkleBranch.push_back(vMerkleTree[j + i]);
		nIndex >>= 1;
	}
	return vMerkleBranch;
}

static inline int ctz(uint32_t i) {
	if (i == 0) return 0;
	int j = 0;
	while (!(i & 1)) {
		j++;
		i >>= 1;
	}
	return j;
}

TEST_CASE("merkle_test")
{
	for (int i = 0; i < 32; i++) {
		// Try 32 block sizes: all sizes from 0 to 16 inclusive, and then 15 random sizes.
		int ntx = (i <= 16) ? i : 17 + (insecure_rand() % 4000);
		// Try up to 3 mutations.
		for (int mutate = 0; mutate <= 3; mutate++) {
			int duplicate1 = mutate >= 1 ? 1 << ctz(ntx) : 0; // The last how many transactions to duplicate first.
			if (duplicate1 >= ntx) break; // Duplication of the entire tree results in a different root (it adds a level).
			int ntx1 = ntx + duplicate1; // The resulting number of transactions after the first duplication.
			int duplicate2 = mutate >= 2 ? 1 << ctz(ntx1) : 0; // Likewise for the second mutation.
			if (duplicate2 >= ntx1) break;
			int ntx2 = ntx1 + duplicate2;
			int duplicate3 = mutate >= 3 ? 1 << ctz(ntx2) : 0; // And for the the third mutation.
			if (duplicate3 >= ntx2) break;
			int ntx3 = ntx2 + duplicate3;
			// Build a block with ntx different transactions.
			CBlock block;
			block.vtx.resize(ntx);
			for (int j = 0; j < ntx; j++) {
				CMutableTransaction mtx;
				mtx.nLockTime = j;
				block.vtx[j] = mtx;
			}
			// Compute the root of the block before mutating it.
			bool unmutatedMutated = false;
			uint256 unmutatedRoot;
			std::tie(unmutatedRoot, unmutatedMutated) = BlockMerkleRoot(block);
			REQUIRE(unmutatedMutated == false);
			// Optionally mutate by duplicating the last transactions, resulting in the same merkle root.
			block.vtx.resize(ntx3);
			for (int j = 0; j < duplicate1; j++) {
				block.vtx[ntx + j] = block.vtx[ntx + j - duplicate1];
			}
			for (int j = 0; j < duplicate2; j++) {
				block.vtx[ntx1 + j] = block.vtx[ntx1 + j - duplicate2];
			}
			for (int j = 0; j < duplicate3; j++) {
				block.vtx[ntx2 + j] = block.vtx[ntx2 + j - duplicate3];
			}
			// Compute the merkle root and merkle tree using the old mechanism.
			bool oldMutated = false;
			std::vector<uint256> merkleTree;
			uint256 oldRoot;
			std::tie(oldRoot, oldMutated, merkleTree) = BlockBuildMerkleTree(block);
			// Compute the merkle root using the new mechanism.
			bool newMutated = false;
			uint256 newRoot;
			std::tie(newRoot, newMutated) = BlockMerkleRoot(block);
			REQUIRE(oldRoot == newRoot);
			REQUIRE(newRoot == unmutatedRoot);
			REQUIRE((newRoot == uint256()) == (ntx == 0));
			REQUIRE(oldMutated == newMutated);
			REQUIRE(newMutated == !!mutate);
			// If no mutation was done (once for every ntx value), try up to 16 branches.
			if (mutate == 0) {
				for (int loop = 0; loop < std::min(ntx, 16); loop++) {
					// If ntx <= 16, try all branches. Otherise, try 16 random ones.
					int mtx = loop;
					if (ntx > 16) {
						mtx = insecure_rand() % ntx;
					}
					std::vector<uint256> newBranch = BlockMerkleBranch(block, mtx);
					std::vector<uint256> oldBranch = BlockGetMerkleBranch(block, merkleTree, mtx);
					REQUIRE(oldBranch == newBranch);
					REQUIRE(ComputeMerkleRootFromBranch(block.vtx[mtx].GetHash(), newBranch, mtx) == oldRoot);
				}
			}
		}
	}
}
