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
	# key_tests.cpp compile error
	limitedmap_tests.cpp
	main_tests.cpp
	mempool_tests.cpp
	miner_tests.cpp
	# multisig_tests.cpp  compile error
	netbase_tests.cpp
	Checkpoints_tests.cpp
	DoS_tests.cpp
	test_ulord.cpp
)
add_executable(test test.cpp ${TEST_SRCS})
target_include_directories(test PRIVATE ../Blockchain)
target_link_libraries(test
PRIVATE Boost::filesystem
PRIVATE Catch2
PUBLIC Blockchain)