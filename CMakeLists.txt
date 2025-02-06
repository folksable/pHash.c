cmake_minimum_required(VERSION 3.15)
project(pHash VERSION 1.0.0 LANGUAGES C)

# Build options
option(BUILD_SHARED_LIB "Build shared library" OFF)
option(BUILD_EXECUTABLE "Build standalone executable" ON)
option(BUILD_TESTS "Build test suite" ON)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Enable interprocedural optimization if available
include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR)
if(IPO_SUPPORTED)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# Platform detection and SIMD flags
include(CheckCCompilerFlag)
set(SIMD_FLAGS)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    set(PHASH_ARCH_ARM64 1)
    check_c_compiler_flag("-mcpu=apple-m1" COMPILER_SUPPORTS_APPLE_M1)
    if(COMPILER_SUPPORTS_APPLE_M1)
        list(APPEND SIMD_FLAGS "-mcpu=apple-m1" "-mtune=apple-m1")
    else()
        check_c_compiler_flag("-march=armv8-a+simd" COMPILER_SUPPORTS_ARM_SIMD)
        if(COMPILER_SUPPORTS_ARM_SIMD)
            list(APPEND SIMD_FLAGS "-march=armv8-a+simd")
        endif()
    endif()
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    set(PHASH_ARCH_X64 1)
    check_c_compiler_flag(-mavx2 COMPILER_SUPPORTS_AVX2)
    check_c_compiler_flag(-msse4.2 COMPILER_SUPPORTS_SSE42)
    check_c_compiler_flag(-mfma COMPILER_SUPPORTS_FMA)

    if(COMPILER_SUPPORTS_AVX2)
        list(APPEND SIMD_FLAGS -mavx2)
    endif()
    if(COMPILER_SUPPORTS_SSE42)
        list(APPEND SIMD_FLAGS -msse4.2)
    endif()
    if(COMPILER_SUPPORTS_FMA)
        list(APPEND SIMD_FLAGS -mfma)
    endif()
endif()

# Common optimization flags
set(OPT_FLAGS
    -O3
    -Wall
    -Wextra
    -fomit-frame-pointer
    -ftree-vectorize
)

# Shared library target
if(BUILD_SHARED_LIB)
    add_library(pHash SHARED pHash.c)

    target_include_directories(pHash PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
    )

    target_compile_options(pHash PRIVATE ${OPT_FLAGS} ${SIMD_FLAGS})
    target_link_libraries(pHash PRIVATE m)

    set_target_properties(pHash PROPERTIES
        MACOSX_RPATH ON
        INSTALL_RPATH "@loader_path/"
    )

    include(GNUInstallDirs)
    install(TARGETS pHash
        EXPORT pHashTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

    install(FILES pHash.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    include(CMakePackageConfigHelpers)
    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/pHashConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pHash
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/pHashConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(EXPORT pHashTargets
        FILE pHashTargets.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pHash
    )

    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/pHashConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/pHashConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pHash
    )
endif()

# Standalone executable target
if(BUILD_EXECUTABLE)
    add_executable(pHash_exec main.c pHash.c)

    target_include_directories(pHash_exec PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_options(pHash_exec PRIVATE ${OPT_FLAGS} ${SIMD_FLAGS})
    target_link_libraries(pHash_exec PRIVATE m pHash)

    set_target_properties(pHash_exec PROPERTIES
        INSTALL_RPATH "@loader_path/../lib"
    )

    include(GNUInstallDirs)
    install(TARGETS pHash_exec
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

# Test target
if(BUILD_TESTS)
    enable_testing()
    add_executable(test_phash test_phash.c pHash.c)

    target_include_directories(test_phash PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_options(test_phash PRIVATE ${OPT_FLAGS} ${SIMD_FLAGS})
    target_link_libraries(test_phash PRIVATE m pHash)

    set_target_properties(test_phash PROPERTIES
        INSTALL_RPATH "@loader_path/../lib"
    )

    add_test(NAME pHash_test COMMAND test_phash)
endif()
