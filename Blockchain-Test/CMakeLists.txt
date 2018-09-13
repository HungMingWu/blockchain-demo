cmake_minimum_required (VERSION 3.8)
find_package(Boost 1.65.0 COMPONENTS filesystem)
set(TEST_SRCS
	#dbwrapper_tests.cpp # TestOK
	claimtriecache_tests.cpp # TestOK
	claimtriebranching_tests.cpp 
	#claimtrie_tests.cpp
	#alert_tests.cpp
	#base58_tests.cpp 
	#accounting_tests.cpp
	#allocator_tests.cpp # TestOK
	#arith_uint256_tests.cpp # TestOK
	#addrman_tests.cpp # TestOK
	#merkle_tests.cpp # TestOK
	#base32_tests.cpp # TestOK
	#base64_tests.cpp # TestOK
	#bip32_tests.cpp # TestOK
	#bloom_tests.cpp # TestOK
	#cachemap_tests.cpp # TestOK
	#cachemultimap_tests.cpp # TestOK
	#coins_tests.cpp
	#crypto_tests.cpp # TestOK
	#compress_tests.cpp # TestOK
	#getarg_tests.cpp
	#hash_tests.cpp # TestOK
	#key_tests.cpp # TestOK
	#limitedmap_tests.cpp # TestOK
	#main_tests.cpp # TestOK
	#mempool_tests.cpp # TestOK
	#miner_tests.cpp
	#multisig_tests.cpp # TestOK
	#netbase_tests.cpp
	#pmt_tests.cpp # TestOK
	#policyestimator_tests.cpp # TestOK
	#pow_tests.cpp # TestOK
	#prevector_tests.cpp # TestOK
	#ratecheck_tests.cpp # TestOK
	#reverselock_tests.cpp # TestOK
	# rpc_tests.cpp
	# rpc_wallet_tests.cpp
	#sanity_tests.cpp
	#scheduler_tests.cpp # TestOK
	#script_P2PKH_tests.cpp # TestOK
	#script_P2SH_tests.cpp # TestOK
	#script_tests.cpp
	#scriptnum_tests.cpp # TestOK
	#serialize_tests.cpp
	# sighash_tests.cpp
	#sigopcount_tests.cpp # TestOK
	#skiplist_tests.cpp # TestOK
	#streams_tests.cpp # TestOK
	#timedata_tests.cpp # TestOK
	#transaction_tests.cpp
	#txvalidationcache_tests.cpp
	#uint256_tests.cpp # TestOK
	#util_tests.cpp  # TestOK
	#versionbits_tests.cpp # TestOK
	#Checkpoints_tests.cpp # TestOK
	#DoS_tests.cpp # TestOK
	test_ulord.cpp
)
add_executable(unit_test test.cpp ${TEST_SRCS})
target_compile_features(unit_test PRIVATE cxx_std_14)
target_include_directories(unit_test PRIVATE ../Blockchain)
target_link_libraries(unit_test
PRIVATE ${Boost_LIBRARIES}
PRIVATE Catch2
PUBLIC Blockchain)
