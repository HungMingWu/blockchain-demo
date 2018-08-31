cmake_minimum_required (VERSION 3.8)
find_package(Boost 1.65.0 COMPONENTS filesystem)
add_executable(demo demo.cpp)
target_include_directories(demo PRIVATE ../Blockchain)
target_link_libraries(demo 
PRIVATE Boost::filesystem
PUBLIC Blockchain)