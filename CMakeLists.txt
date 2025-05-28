if( MONK )
    set( CMAKE_C_COMPILER   "${LLVM_BUILD}/bin/clang" )
    set( CMAKE_CXX_COMPILER "${LLVM_BUILD}/bin/clang++" )
    set( CMAKE_CXX_FLAGS    "${CMAKE_CXX_FLAGS} -stdlib=libc++" )
    set( CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld -Wl,-rpath -Wl,${LLVM_BUILD}/lib/x86_64-unknown-linux-gnu" )
    set( CMAKE_INSTALL_RPATH "${LLVM_BUILD}/lib/x86_64-pc-linux-gnu" )
    set( CMAKE_BUILD_RPATH   "${LLVM_BUILD}/lib/x86_64-pc-linux-gnu" )
endif()


# CMake 3.30 is required for C++23 `import std` support; we use 3.29.20240416
# here so that in-development versions satisfy it. 
cmake_minimum_required(VERSION 3.29.20240416 FATAL_ERROR)

# Set experimental flag to enable `import std` support from CMake.
# This must be enabled before C++ language support.
set(
    CMAKE_EXPERIMENTAL_CXX_IMPORT_STD
    # This specific value changes as experimental support evolves. See
    # `Help/dev/experimental.rst` in the CMake source corresponding to
    # your CMake build for the exact value to use.
    "a9e1cf81-9932-4810-974b-6eccaf14e457"
)

project( mhfem-diffprog LANGUAGES C CXX )

set( CMAKE_CXX_MODULE_STD ON )

# set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -pedantic -Wno-deprecated-declarations" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wno-deprecated-declarations" )

set( CMAKE_CXX_STANDARD          23 )
set( CMAKE_CXX_EXTENSIONS        ON )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

include( CPM.cmake/cmake/CPM.cmake )

# Personal lib
CPMAddPackage( "gh:gregthemadmonk/dot-xx#main" )
# Google benchmarking
set( BENCHMARK_ENABLE_TESTING OFF )
CPMAddPackage( "gh:google/benchmark#main" )

file( GLOB_RECURSE XX_SRC "src/*.xx" "test/*.xx" )
set_source_files_properties( ${XX_SRC} PROPERTIES LANGUAGE CXX )

function( add_module_library LIB_NAME )
    string( REPLACE "." "/" LIB_DIR "${LIB_NAME}" )
    string( REPLACE "." "_" LIB_TAR "${LIB_NAME}" )

    file( GLOB_RECURSE LIB_CC "src/${LIB_DIR}/*.cc" )
    file( GLOB_RECURSE LIB_XX "src/${LIB_DIR}/*.xx" )

    set_source_files_properties( ${LIB_XX} PROPERTIES LANGUAGE CXX )

    add_library( "${LIB_NAME}" STATIC ${LIB_CC} )
    target_sources(
        "${LIB_NAME}" PUBLIC
        FILE_SET "${LIB_TAR}_mod"
        TYPE CXX_MODULES
        FILES ${LIB_XX}
    )

    target_compile_features( "${LIB_NAME}" PUBLIC cxx_std_23 )
endfunction()

add_module_library( utils )
target_link_libraries( utils PRIVATE dot-xx::all )
add_module_library( math )
target_link_libraries( math PRIVATE dot-xx::all utils )
add_module_library( mesh )
target_link_libraries( mesh PRIVATE dot-xx::all utils )
add_module_library( mhfe )
target_link_libraries( mhfe PRIVATE dot-xx::all math utils mesh )

add_executable( meshgen "src/meshgen/main.cc" )
target_compile_features( meshgen PRIVATE cxx_std_23 )
target_link_libraries( meshgen PRIVATE mesh utils dot-xx::all )

add_executable( vis "src/vis/vis.cc" )
target_compile_features( vis PRIVATE cxx_std_23 )
target_link_libraries( vis PRIVATE mesh mhfe utils dot-xx::all )

file( GLOB_RECURSE BENCH_CC "bench/*.cc" )
add_executable( bench ${BENCH_CC} )
target_compile_features( bench PRIVATE cxx_std_23 )
target_link_libraries(
    bench PRIVATE
    benchmark::benchmark mhfe math mesh utils dot-xx::all
)

file( GLOB_RECURSE SELFTEST_CC "test/*.cc" )
file( GLOB_RECURSE SELFTEST_XX "test/*.xx" )
add_executable( selftest ${SELFTEST_CC} )
target_sources(
    selftest PRIVATE
    FILE_SET selftest_mod
    TYPE     CXX_MODULES
    FILES    ${SELFTEST_XX}
)
target_compile_features( selftest PRIVATE cxx_std_23 )
target_link_libraries( selftest PRIVATE mhfe math mesh utils dot-xx::all )
