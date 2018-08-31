cmake_minimum_required (VERSION 3.8)
find_package(Boost 1.65.0 COMPONENTS 
filesystem date_time program_options thread
serialization)
find_package(leveldb REQUIRED)
find_package(openssl REQUIRED)
find_package(Libevent CONFIG REQUIRED)
find_package(Secp256k1 REQUIRED)
file(GLOB_RECURSE Source_List "*.cpp" "*.c" "*.hpp" "*.h")
add_library (Blockchain ${Source_List})
target_link_libraries(Blockchain 
PUBLIC Boost::filesystem
PRIVATE Boost::date_time
PRIVATE Boost::program_options
PRIVATE Boost::thread
PRIVATE Boost::serialization
PRIVATE event event_core event_extra
PUBLIC spdlog
PRIVATE ${LevelDB_LIBRARY}
PRIVATE ${OPENSSL_LIBRARIES}
PRIVATE ${Secp256k1_LIBRARY}
)

target_compile_options(Blockchain PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
        -pipe -march=native
        $<$<CONFIG:Debug>:-O0 -Wall -Wextra>>
    $<$<CXX_COMPILER_ID:MSVC>:
        $<$<CONFIG:Debug>:/bigobj>>
)

if (WIN32)
target_link_libraries(Blockchain 
PRIVATE Ws2_32.lib
PRIVATE Shlwapi.lib
)
endif()