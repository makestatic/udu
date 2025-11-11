cmake_minimum_required(VERSION 3.15)
project(udu VERSION 1.0.10 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON) # (gnu)

option(ENABLE_OPENMP "Enable OpenMP support" ON)
option(ENABLE_LTO "Enable Link Time Optimization" ON)

set(UDU_SOURCES
    c/udu.c
)

add_executable(udu ${UDU_SOURCES})

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    # Use -O2 instead of -O3 due to known bugs with -O3
    # - GCC 15.2.1: overwrite opts.verbose
    # - Clang 21.1.5: sigfault
    target_compile_options(udu PRIVATE
        -Wall
        -Wextra
		-Wshadow
		-Wconversion
        -pedantic
        -O2
    )
    
    # cpu specific optimizations
	if(CMAKE_BUILD_TYPE MATCHES "Release|RelWithDebInfo")
        target_compile_options(udu PRIVATE -mtune=native -mcpu=native -march=native)
    endif()
elseif(MSVC)
    target_compile_options(udu PRIVATE
        /W4
        /O2
    )
endif()

if(ENABLE_OPENMP)
    find_package(OpenMP)
    if(OpenMP_C_FOUND)
        target_link_libraries(udu PRIVATE OpenMP::OpenMP_C)
        message(STATUS "OpenMP enabled")
    else()
        message(WARNING "OpenMP not found, building without parallel support")
    endif()
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

if(WIN32)
    # Windows
    target_compile_definitions(udu PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    # Unix
    target_compile_definitions(udu PRIVATE 
        _XOPEN_SOURCE=700
        _POSIX_C_SOURCE=200809L
		_FORTIFY_SOURCE=2
		_GNU_SOURCE
    )
endif()

include(GNUInstallDirs)
install(TARGETS udu
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

#  build information
message(STATUS "========================================")
message(STATUS "udu build configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "  OpenMP: ${ENABLE_OPENMP}")
message(STATUS "  LTO: ${ENABLE_LTO}")
message(STATUS "========================================")

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
        "Choose the type of build: Debug Release RelWithDebInfo MinSizeRel"
        FORCE)
endif()

if(CMAKE_BUILD_TYPE MATCHES "Debug|RelWithDebInfo")
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(udu PRIVATE -g)
    elseif(MSVC)
        target_compile_options(udu PRIVATE /Zi)
    endif()
endif()
