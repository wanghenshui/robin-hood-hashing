cmake_minimum_required(VERSION 3.5)
project(robin-hood-hashing
    VERSION 3.11.5
    LANGUAGES CXX
)

# generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(RH_BENCHMARK ON)
include(cmake/CPM.cmake)

if (NOT EXISTS ${CMAKE_BINARY_DIR}/CMakeCache.txt)
  if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
  endif()
endif()

# determine whether this is a standalone project or included by other projects
if (NOT DEFINED RH_STANDALONE_PROJECT)
    if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        set(RH_STANDALONE_PROJECT ON)
    else()
        set(RH_STANDALONE_PROJECT OFF)
    endif ()
endif()

if (RH_STANDALONE_PROJECT)
    option(RH_sanitizer "Address sanitizer" OFF)
    option(RH_coverage "Enable coverage" OFF)
    set(RH_cxx_standard "20" CACHE STRING "C++ standard, e.g. 11, 14, 17")

    set(CMAKE_CXX_STANDARD ${RH_cxx_standard})
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)

    # configuration see .clang-tidy
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        find_program(CLANG_TIDY_PROGRAM NAMES clang-tidy)
        if(CLANG_TIDY_PROGRAM)
            set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_PROGRAM}")
        endif()
    endif()

    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        SET(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        #SET_PROPERTY(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "/home/martinus/git/rust-throttler/target/release/throttler run -g compile --")
        #SET_PROPERTY(GLOBAL PROPERTY RULE_LAUNCH_LINK "/home/martinus/git/rust-throttler/target/release/throttler run -g link --")
    endif()

    add_executable(rh "")

    if (RH_no_exceptions)
        target_compile_options(rh PRIVATE -fno-exceptions)
        target_link_libraries(rh PRIVATE -fno-exceptions)
        add_definitions(-DDOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS)
    endif()

    if (RH_sanitizer)
        if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            # see https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html#silencing-unsigned-integer-overflow
            # Compile with -g and -fno-omit-frame-pointer to get proper debug information in your binary
            target_compile_options(rh PRIVATE -g)
            target_compile_options(rh PRIVATE -O2)
            target_compile_options(rh PRIVATE -fno-omit-frame-pointer)

            target_compile_options(rh PRIVATE -fsanitize=address)
            target_link_libraries(rh PRIVATE -fsanitize=address)

            target_compile_options(rh PRIVATE -fsanitize=undefined)
            target_link_libraries(rh PRIVATE -fsanitize=undefined)

            target_compile_options(rh PRIVATE -fsanitize=integer)
            target_compile_options(rh PRIVATE -fno-sanitize=unsigned-integer-overflow)
            target_link_libraries(rh PRIVATE -fsanitize=integer)
            target_link_libraries(rh PRIVATE -fno-sanitize=unsigned-integer-overflow)

            target_compile_options(rh PRIVATE -fsanitize=nullability)
            target_link_libraries(rh PRIVATE -fsanitize=nullability)
        endif()
        if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            # need to use gold linker, otherwise travis gets '/usr/bin/ld: --push-state: unknown option' error
            target_link_libraries(rh PRIVATE -fuse-ld=gold)

            target_compile_options(rh PRIVATE -g)
            target_compile_options(rh PRIVATE -O2)
            target_compile_options(rh PRIVATE -fno-omit-frame-pointer)

            target_compile_options(rh PRIVATE -fsanitize=undefined,float-divide-by-zero,float-cast-overflow)
            target_link_libraries(rh PRIVATE -fsanitize=undefined,float-divide-by-zero,float-cast-overflow)

            target_compile_options(rh PRIVATE -fsanitize=pointer-compare,pointer-subtract)
            target_link_libraries(rh PRIVATE -fsanitize=pointer-compare,pointer-subtract)

            target_compile_options(rh PRIVATE -fsanitize=address)
            target_link_libraries(rh PRIVATE -fsanitize=address)
        endif()
    endif()

    if(RH_coverage AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        MESSAGE(STATUS "Coverage enable")

        # Add required flags (GCC & LLVM/Clang)
        target_compile_options(rh PRIVATE -fprofile-arcs -ftest-coverage)
        target_link_libraries(rh PRIVATE -fprofile-arcs -ftest-coverage)

        # find the correct gcov version
        #get_filename_component(COMPILER_PATH "${CMAKE_CXX_COMPILER}" PATH)
        #string(REGEX MATCH "^[0-9]+" GCC_VERSION "${CMAKE_CXX_COMPILER_VERSION}")
        #find_program(GCOV_BIN NAMES gcov-${GCC_VERSION} gcov HINTS ${COMPILER_PATH})

        # collect all source files from the chosen include dir
        file(GLOB_RECURSE SOURCE_FILES src/include/robin_hood.h)

        # COMMAND ${CMAKE_SOURCE_DIR}/test/thirdparty/imapdl/filterbr.py json.info.filtered > json.info.filtered.noexcept
        add_custom_target(lcov
            COMMAND lcov --directory . --capture --output-file coverage.info # parse coverage data
            COMMAND lcov -e coverage.info ${SOURCE_FILES} --output-file coverage.info # filter
            COMMAND coveralls-lcov coverage.info  # upload results
        )

    endif()

    #set_target_properties(rh PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
    #set_target_properties(rh PROPERTIES COMPILE_FLAGS "-m64" LINK_FLAGS "-m64")

    add_subdirectory(cmake)

    add_compile_flags_target(rh)
    add_subdirectory(src)

    target_sources_local(rh PUBLIC .clang-tidy)
else()
    # creates a library robin_hood which is an interface (header files only)

    set(LIB_NAME "robin_hood")
    set(LIB_NAMESPACE "robin_hood")

    add_library(${LIB_NAME} INTERFACE)
    add_library(${LIB_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

    target_include_directories(robin_hood INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/include>
        $<INSTALL_INTERFACE:include>
    )

    include(GNUInstallDirs)

    install(
        TARGETS ${LIB_NAME}
        EXPORT ${LIB_NAME}-targets
    )

    install(
        FILES src/include/robin_hood.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    configure_file(
        cmake/config.cmake.in
        ${LIB_NAME}Config.cmake
        @ONLY
    )

    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/${LIB_NAME}Config.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${LIB_NAME}
    )

    install(
        EXPORT ${LIB_NAME}-targets
        NAMESPACE ${LIB_NAMESPACE}::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${LIB_NAME}
    )
endif()

if (RH_BENCHMARK)

include(cmake/CPM.cmake)
CPMAddPackage(
  NAME benchmark
  GITHUB_REPOSITORY google/benchmark
  VERSION 1.8.3
  OPTIONS "BENCHMARK_ENABLE_TESTING Off"
)

#CPMAddPackage(
#  NAME Boost
#  VERSION 1.87.0 # Versions less than 1.85.0 may need patches for installation targets.
#  URL https://archives.boost.io/release/1.87.0/source/boost_1_87_0.tar.gz
#  URL_HASH SHA256=f55c340aa49763b1925ccf02b2e83f35fdcf634c9d5164a2acb87540173c741d
#  OPTIONS "BOOST_ENABLE_CMAKE ON" "BOOST_SKIP_INSTALL_RULES ON" # Set `OFF` for installation
#          "BUILD_SHARED_LIBS OFF" "BOOST_INCLUDE_LIBRARIES container\\\;asio" # Note the escapes!
#)


if(benchmark_ADDED)
  # enable c++11 to avoid compilation errors
  set_target_properties(benchmark PROPERTIES CXX_STANDARD 17)
endif()	
include_directories(src/include)

add_executable(bm_find "benchmark/find.cc")
target_link_libraries(bm_find benchmark)
#target_link_libraries(bm_find PRIVATE Boost::headers Boost::container)
target_compile_features(bm_find PRIVATE cxx_std_20)

endif()
