cmake_minimum_required(VERSION 3.15)
project(pHash VERSION 1.0.0 LANGUAGES C)

# Configuration options
option(PHASH_BUILD_EXAMPLES "Build example executables" ON)
option(PHASH_ENABLE_SIMD "Enable architecture-specific SIMD optimizations" ON)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Platform detection
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    set(PHASH_ARCH_ARM64 1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    set(PHASH_ARCH_X64 1)
endif()

# Compiler flags
set(PHASH_COMPILE_OPTIONS)
set(PHASH_LINK_OPTIONS)

if(PHASH_ENABLE_SIMD)
    include(CheckCCompilerFlag)
    
    if(PHASH_ARCH_X64)
        check_c_compiler_flag(-mavx2 COMPILER_SUPPORTS_AVX2)
        check_c_compiler_flag(-msse4.2 COMPILER_SUPPORTS_SSE42)
        
        if(COMPILER_SUPPORTS_AVX2 AND COMPILER_SUPPORTS_SSE42)
            list(APPEND PHASH_COMPILE_OPTIONS -mavx2 -msse4.2)
        endif()
        
    elseif(PHASH_ARCH_ARM64)
        check_c_compiler_flag("-mcpu=apple-m1" COMPILER_SUPPORTS_APPLE_M1)
        check_c_compiler_flag(-march=armv8-a+simd COMPILER_SUPPORTS_ARM_SIMD)
        
        if(COMPILER_SUPPORTS_APPLE_M1)
            list(APPEND PHASH_COMPILE_OPTIONS "-mcpu=apple-m1" "-mtune=apple-m1")
        elseif(COMPILER_SUPPORTS_ARM_SIMD)
            list(APPEND PHASH_COMPILE_OPTIONS -march=armv8-a+simd)
        endif()
    endif()
endif()

# Common optimization flags
list(APPEND PHASH_COMPILE_OPTIONS
    -O3
    -Wall
    -Wextra
)

# Library target
add_library(pHash
    pHash.c
)

target_include_directories(pHash PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_compile_options(pHash PRIVATE ${PHASH_COMPILE_OPTIONS})
target_link_libraries(pHash PRIVATE m)

# Install rules
include(GNUInstallDirs)

install(TARGETS pHash
    EXPORT pHashTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES pHash.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Example executable
if(PHASH_BUILD_EXAMPLES)
    add_executable(pHash_demo main.c)
    target_compile_options(pHash_demo PRIVATE ${PHASH_COMPILE_OPTIONS})
    target_link_libraries(pHash_demo PRIVATE pHash m)
    
    install(TARGETS pHash_demo
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

# Package config
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