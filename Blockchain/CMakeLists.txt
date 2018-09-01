cmake_minimum_required (VERSION 3.8)
find_package(Boost 1.65.0 REQUIRED COMPONENTS 
filesystem date_time program_options thread
serialization)
find_package(LevelDB REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Libevent CONFIG REQUIRED)
find_package(Secp256k1 REQUIRED)
file(GLOB_RECURSE Source_List "*.cpp" "*.c" "*.hpp" "*.h")
add_library (Blockchain ${Source_List})
target_include_directories(Blockchain PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(Blockchain 
PRIVATE ${Boost_LIBRARIES}
PUBLIC spdlog
PRIVATE ${LevelDB_LIBRARY}
PRIVATE ${OPENSSL_LIBRARIES}
PRIVATE ${Secp256k1_LIBRARY}
PRIVATE event event_core event_extra
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
