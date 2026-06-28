cmake_minimum_required(VERSION 3.23)

project(lowlat_matching_engine LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(LOWLAT_BUILD_TESTS "Build lowlat assertion test executables" ON)
option(LOWLAT_BUILD_BENCHMARKS "Build Google Benchmark executables" ON)
option(LOWLAT_BUILD_TOOLS "Build profiling tools" ON)

add_library(lowlat INTERFACE)
target_compile_features(lowlat INTERFACE cxx_std_20)
target_include_directories(lowlat INTERFACE
    "${PROJECT_SOURCE_DIR}/src"
)

add_library(lowlat_options INTERFACE)
if(MSVC)
    target_compile_options(lowlat_options INTERFACE /W4)
else()
    target_compile_options(lowlat_options INTERFACE -Wall -Wextra)
endif()

add_library(lowlat_asserts_enabled INTERFACE)
if(MSVC)
    target_compile_options(lowlat_asserts_enabled INTERFACE /UNDEBUG)
else()
    target_compile_options(lowlat_asserts_enabled INTERFACE -UNDEBUG)
endif()

add_library(lowlat_avx2 INTERFACE)
if(MSVC)
    target_compile_options(lowlat_avx2 INTERFACE /arch:AVX2)
else()
    target_compile_options(lowlat_avx2 INTERFACE -mavx2)
endif()

find_package(Threads REQUIRED)

function(lowlat_add_executable target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE lowlat lowlat_options Threads::Threads)
endfunction()

function(lowlat_add_test target source)
    lowlat_add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE lowlat_asserts_enabled)
    add_test(NAME ${target} COMMAND ${target})
endfunction()

function(lowlat_add_benchmark target source)
    lowlat_add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE benchmark::benchmark)
endfunction()

function(lowlat_add_tool target source)
    lowlat_add_executable(${target} ${source})
endfunction()

if(LOWLAT_BUILD_TESTS)
    include(CTest)

    lowlat_add_test(test_order_pool tests/matching/test_order_pool.cpp)
    lowlat_add_test(test_fixed_clob tests/matching/test_fixed_clob.cpp)
    lowlat_add_test(test_order_index tests/matching/test_order_index.cpp)
    lowlat_add_test(test_matching tests/matching/test_matching.cpp)
    lowlat_add_test(test_spsc_ring tests/infra/test_spsc_ring.cpp)
    lowlat_add_test(test_spsc_cached tests/infra/test_spsc_cached.cpp)

    lowlat_add_test(test_simd_scan tests/infra/test_simd_scan.cpp)
    target_link_libraries(test_simd_scan PRIVATE lowlat_avx2)
endif()

if(LOWLAT_BUILD_BENCHMARKS)
    find_package(benchmark CONFIG QUIET)

    if(NOT TARGET benchmark::benchmark)
        set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
        set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
        set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

        if(EXISTS "${PROJECT_SOURCE_DIR}/../benchmark/CMakeLists.txt")
            add_subdirectory("${PROJECT_SOURCE_DIR}/../benchmark" benchmark_build EXCLUDE_FROM_ALL)
        else()
            message(FATAL_ERROR
                "Google Benchmark was not found. Install benchmark, place it at ../benchmark, "
                "or configure with -DLOWLAT_BUILD_BENCHMARKS=OFF."
            )
        endif()
    endif()

    lowlat_add_benchmark(bench_fixed_clob bench/matching/bench_fixed_clob.cpp)
    lowlat_add_benchmark(bench_matching bench/matching/bench_matching.cpp)
    lowlat_add_benchmark(bench_matching_hot bench/matching/bench_matching_hot.cpp)
    lowlat_add_benchmark(bench_order_index bench/matching/bench_order_index.cpp)
    lowlat_add_benchmark(bench_pipeline bench/pipeline/bench_pipeline.cpp)
    target_link_libraries(bench_pipeline PRIVATE lowlat_asserts_enabled)

    lowlat_add_benchmark(bench_simd_scan bench/infra/bench_simd_scan.cpp)
    target_link_libraries(bench_simd_scan PRIVATE lowlat_avx2 lowlat_asserts_enabled)
endif()

if(LOWLAT_BUILD_TOOLS)
    lowlat_add_tool(profile_full_match tools/matching/profile_full_match.cpp)
    lowlat_add_tool(profile_full_match_no_exec tools/matching/profile_full_match_no_exec.cpp)
    lowlat_add_tool(profile_full_match_raw_exec tools/matching/profile_full_match_raw_exec.cpp)
    lowlat_add_tool(profile_pipeline_latency tools/pipeline/profile_pipeline_latency.cpp)
    target_link_libraries(profile_pipeline_latency PRIVATE lowlat_asserts_enabled)
endif()
