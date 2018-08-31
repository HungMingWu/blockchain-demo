cmake_minimum_required (VERSION 3.8)
find_package(Boost 1.65.0 COMPONENTS filesystem)
set(TEST_SRCS
	dbwrapper_tests.cpp
	#claimtrie_tests.cpp
	merkle_tests.cpp
	base32_tests.cpp
)
add_executable(test test.cpp ${TEST_SRCS})
target_include_directories(test PRIVATE ../Blockchain)
target_link_libraries(test
PRIVATE Boost::filesystem
PRIVATE Catch2
PUBLIC Blockchain)