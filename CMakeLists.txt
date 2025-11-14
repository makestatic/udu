cmake_minimum_required(VERSION 3.15)
project(udu VERSION 2.0.2 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(ENABLE_OPENMP "Enable Parallel Processing" ON)
option(ENABLE_LTO "Enable Link Time Optimization" ON)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
	"Choose the type of build: Debug Release RelWithDebInfo MinSizeRel" FORCE)
endif()

add_executable(udu c/udu.c)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
	target_compile_options(udu PRIVATE
		-Wall -Wextra -Wshadow -Wconversion -pedantic
		$<$<CONFIG:Release>:-march=native -mtune=native>
		$<$<CONFIG:RelWithDebInfo>:-march=native -mtune=native>
	)
elseif(MSVC)
	target_compile_options(udu PRIVATE
		/W4 /permissive- /volatile:iso /EHsc
	)
endif()

if(ENABLE_OPENMP AND NOT MSVC)
	find_package(OpenMP 3.1 QUIET)
	if(OpenMP_C_FOUND)
		target_link_libraries(udu PRIVATE OpenMP::OpenMP_C)
		message(STATUS "OpenMP enabled (version: ${OpenMP_C_VERSION})")
	else()
		message(WARNING "OpenMP not found, building without parallel support")
	endif()
else()
	message(WARNING "MSVC lacks support for OpenMP 3.1")
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
