cmake_minimum_required (VERSION 3.8)
find_package(Boost 1.65.0 COMPONENTS filesystem)
set(TEST_SRCS
	dbwrapper_tests.cpp
	claimtrie_tests.cpp
	#alert_tests.cpp
	#base58_tests.cpp 
	accounting_tests.cpp
	allocator_tests.cpp
	arith_uint256_tests.cpp
	addrman_tests.cpp 
	merkle_tests.cpp
	base32_tests.cpp
	base64_tests.cpp
	bip32_tests.cpp
	bloom_tests.cpp
	cachemap_tests.cpp
	cachemultimap_tests.cpp
	checkblock_tests.cpp
	coins_tests.cpp
	crypto_tests.cpp
	compress_tests.cpp
	getarg_tests.cpp
	hash_tests.cpp
	key_tests.cpp
	limitedmap_tests.cpp
	main_tests.cpp
	mempool_tests.cpp
	miner_tests.cpp
	multisig_tests.cpp
	netbase_tests.cpp
	pmt_tests.cpp
	policyestimator_tests.cpp
	pow_tests.cpp
	prevector_tests.cpp
	ratecheck_tests.cpp
	reverselock_tests.cpp
	# rpc_tests.cpp
	# rpc_wallet_tests.cpp
	sanity_tests.cpp
	scheduler_tests.cpp
	script_P2PKH_tests.cpp
	script_P2SH_tests.cpp
	# script_tests.cpp
	scriptnum_tests.cpp
	serialize_tests.cpp
	# sighash_tests.cpp
	sigopcount_tests.cpp
	skiplist_tests.cpp
	streams_tests.cpp
	timedata_tests.cpp
	#transaction_tests.cpp
	txvalidationcache_tests.cpp
	uint256_tests.cpp
	util_tests.cpp
	versionbits_tests.cpp
	Checkpoints_tests.cpp
	DoS_tests.cpp
	test_ulord.cpp
)
add_executable(unit_test test.cpp ${TEST_SRCS})
target_compile_features(unit_test PRIVATE cxx_std_14)
target_include_directories(unit_test PRIVATE ../Blockchain)
target_link_libraries(unit_test
PRIVATE ${Boost_LIBRARIES}
PRIVATE Catch2
PUBLIC Blockchain)
