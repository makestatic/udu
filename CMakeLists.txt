cmake_minimum_required(VERSION 3.15)
project(udu VERSION 0.4.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(ENABLE_OPENMP "Enable Parallel Processing" ON)
option(ENABLE_LTO "Enable Link Time Optimization" ON)
option(STATIC_OPENMP "Link OpenMP statically" OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
        "Choose the type of build: Debug Release RelWithDebInfo MinSizeRel" FORCE)
endif()

add_executable(udu
    C/udu.c C/types.h
    C/platform.c C/platform.h
    C/util.c C/util.h
    C/xxhash.h
)
target_compile_definitions(udu PRIVATE VERSION="${PROJECT_VERSION}")

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(udu PRIVATE -Wall -Wextra -Wunused -Wshadow -Wconversion -pedantic)
elseif(MSVC)
    target_compile_options(udu PRIVATE /W4 /permissive- /volatile:iso /EHsc)
endif()

# using llvm may lead to runtime crash (see #2)
if(CMAKE_C_COMPILER_ID MATCHES "Clang" AND CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    message(WARNING "Compiling on AArch64 using LLVM toolchain may lead to runtime crashes. Use GCC for stable builds.\nMore info: https://github.com/makestatic/udu/issues/2")
endif()

if(ENABLE_OPENMP AND NOT MSVC)
    find_package(OpenMP 3.1 QUIET)
    if(OpenMP_C_FOUND)
        if(STATIC_OPENMP AND (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang"))
            # Search for static openmp 
            file(GLOB_RECURSE LIBGOMP_STATIC "/usr/lib/gcc/*/*/libgomp.a")
            file(GLOB_RECURSE LIBOMP_STATIC "/usr/lib/llvm-*/lib/libomp.a")

            if(LIBGOMP_STATIC)
                list(GET LIBGOMP_STATIC 0 LIBGOMP_STATIC_FIRST)
                target_link_libraries(udu PRIVATE ${LIBGOMP_STATIC_FIRST} pthread)
                message(STATUS "OpenMP enabled (static libgomp)")
            elseif(LIBOMP_STATIC)
                list(GET LIBOMP_STATIC 0 LIBOMP_STATIC_FIRST)
                target_link_libraries(udu PRIVATE ${LIBOMP_STATIC_FIRST} pthread)
                message(STATUS "OpenMP enabled (static libomp)")
            else()
                target_link_libraries(udu PRIVATE OpenMP::OpenMP_C)
                message(WARNING "Static OpenMP runtime not found; linking dynamically")
            endif()
        else()
            target_link_libraries(udu PRIVATE OpenMP::OpenMP_C)
            message(STATUS "OpenMP enabled (dynamic linking)")
        endif()
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
