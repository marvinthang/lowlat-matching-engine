cmake_minimum_required(VERSION 3.23)

project(lowlat_matching_engine LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BENCHMARK_ENABLE_TESTING OFF)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF)

add_subdirectory(../benchmark benchmark_build)

add_library(lowlat INTERFACE)
target_include_directories(lowlat INTERFACE src)

add_executable(test_order_pool tests/test_order_pool.cpp)
target_link_libraries(test_order_pool PRIVATE lowlat benchmark::benchmark)
target_compile_options(test_order_pool PRIVATE -Wall -Wextra -UNDEBUG)

add_executable(test_fixed_clob tests/test_fixed_clob.cpp)
target_link_libraries(test_fixed_clob PRIVATE lowlat benchmark::benchmark)
target_compile_options(test_fixed_clob PRIVATE -Wall -Wextra -UNDEBUG)

add_executable(bench_fixed_clob bench/bench_fixed_clob.cpp)
target_link_libraries(bench_fixed_clob PRIVATE lowlat benchmark::benchmark)
target_compile_options(bench_fixed_clob PRIVATE -Wall -Wextra)

add_executable(test_order_index tests/test_order_index.cpp)
target_link_libraries(test_order_index PRIVATE lowlat benchmark::benchmark)
target_compile_options(test_order_index PRIVATE -Wall -Wextra -UNDEBUG)

add_executable(test_matching tests/test_matching.cpp)
target_link_libraries(test_matching PRIVATE lowlat benchmark::benchmark)
target_compile_options(test_matching PRIVATE -Wall -Wextra -UNDEBUG)

add_executable(bench_matching bench/bench_matching.cpp)
target_link_libraries(bench_matching PRIVATE lowlat benchmark::benchmark)
target_compile_options(bench_matching PRIVATE -Wall -Wextra)

add_executable(bench_matching_hot bench/bench_matching_hot.cpp)
target_link_libraries(bench_matching_hot PRIVATE lowlat benchmark::benchmark)
target_compile_options(bench_matching_hot PRIVATE -Wall -Wextra)

add_executable(bench_order_index bench/bench_order_index.cpp)
target_link_libraries(bench_order_index PRIVATE lowlat benchmark::benchmark)
target_compile_options(bench_order_index PRIVATE -Wall -Wextra)

add_executable(profile_full_match tools/profile_full_match.cpp)
target_link_libraries(profile_full_match PRIVATE lowlat)
target_compile_options(profile_full_match PRIVATE -Wall -Wextra)

add_executable(profile_full_match_no_exec tools/profile_full_match_no_exec.cpp)
target_link_libraries(profile_full_match_no_exec PRIVATE lowlat)
target_compile_options(profile_full_match_no_exec PRIVATE -Wall -Wextra)

add_executable(profile_full_match_raw_exec tools/profile_full_match_raw_exec.cpp)
target_link_libraries(profile_full_match_raw_exec PRIVATE lowlat)
target_compile_options(profile_full_match_raw_exec PRIVATE -Wall -Wextra)

add_executable(test_spsc_ring tests/test_spsc_ring.cpp)
target_link_libraries(test_spsc_ring PRIVATE lowlat benchmark::benchmark)
target_compile_options(test_spsc_ring PRIVATE -Wall -Wextra -UNDEBUG)

add_executable(bench_pipeline bench/bench_pipeline.cpp)
target_link_libraries(bench_pipeline PRIVATE lowlat benchmark::benchmark)
target_compile_options(bench_pipeline PRIVATE -Wall -Wextra -UNDEBUG)

add_executable(test_spsc_cached tests/test_spsc_cached.cpp)
target_link_libraries(test_spsc_cached PRIVATE lowlat benchmark::benchmark)
target_compile_options(test_spsc_cached PRIVATE -Wall -Wextra -UNDEBUG)

add_executable(test_simd_scan tests/test_simd_scan.cpp)
target_link_libraries(test_simd_scan PRIVATE lowlat)
target_compile_options(test_simd_scan PRIVATE -Wall -Wextra -mavx2 -UNDEBUG)

add_executable(bench_simd_scan bench/bench_simd_scan.cpp)
target_link_libraries(bench_simd_scan PRIVATE lowlat benchmark::benchmark)
target_compile_options(bench_simd_scan PRIVATE -Wall -Wextra -mavx2 -UNDEBUG)
