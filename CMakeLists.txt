cmake_minimum_required(VERSION 3.15)
project(udu VERSION 0.6.1 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(ENABLE_OPENMP "Enable Parallel Processing" ON)
option(ENABLE_LTO "Enable Link Time Optimization" ON)

# default to RelWithDebInfo build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()

add_executable(udu
    C/main.c C/args.c C/walk.c
    C/platform.c C/util.c
)
target_compile_definitions(udu PRIVATE VERSION="${PROJECT_VERSION}")

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(udu PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wunused
        -Wshadow
        -Wconversion
        -Wsign-conversion
        -Wformat=2
        -Wformat-security
        -Wnull-dereference
        -Wdouble-promotion
        -Wimplicit-fallthrough
        # $<$<CONFIG:Release>:-march=native>
    )

    target_compile_definitions(udu PRIVATE
		#_POSIX_C_SOURCE=200809L
        #_FORTIFY_SOURCE=2
    )
elseif(MSVC)
    target_compile_options(udu PRIVATE
        /W4
        /permissive-
        /volatile:iso
        /EHsc
        /Zc:inline
        /Zc:preprocessor
    )
endif()

# using llvm may lead to runtime crash (see #2)
if(CMAKE_C_COMPILER_ID MATCHES "Clang" AND CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    message(WARNING 
        "Compiling with Clang on AArch64 may cause runtime crashes. "
        "Consider using GCC for stable builds. "
        "See: https://github.com/makestatic/udu/issues/2"
    )
endif()

if(ENABLE_OPENMP)
    find_package(OpenMP 3.1 QUIET)
    if(OpenMP_C_FOUND)
            target_link_libraries(udu PRIVATE OpenMP::OpenMP_C)
            message(STATUS "OpenMP enabled (dynamic linking)")
    else()
        message(WARNING "OpenMP not found; building without parallel processing")
    endif()
elseif(MSVC)
    message(ERROR "MSVC lacks support for OpenMP 3.1")
endif()

if(ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
    if(ipo_supported)
        set_property(TARGET udu PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
        message(STATUS "LTO enabled")
    else()
        message(WARNING "LTO not supported: ${ipo_error}")
    endif()
endif()

include(GNUInstallDirs)
install(TARGETS udu RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
