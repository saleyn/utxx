# vim:ts=2:sw=2:et
cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
project(utxx VERSION 1.4)

#===============================================================================
# CMAKE options customization
#===============================================================================
option(WITH_THRIFT             "Enable to compile UTXX with Thrift"          ON)
option(VERBOSE                 "Turn verbosity on|off"                      OFF)
option(WITH_ENUM_SERIALIZATION "Turn enum serialization support on|off"     OFF)

if(VERBOSE)
  set(CMAKE_VERBOSE_MAKEFILE ON)
endif()
if(WITH_ENUM_SERIALIZATION)
  set(UTXX_ENUM_SUPPORT_SERIALIZATION ON)
endif()

string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE)
string(TOLOWER ${TOOLCHAIN} toolchain)

# Custom extensions
include(${CMAKE_CURRENT_SOURCE_DIR}/build-aux/CMakeEx.txt)

#-------------------------------------------------------------------------------
# Toolchain
#-------------------------------------------------------------------------------
# See also build/CMakeInit.txt
if("${toolchain}" STREQUAL "gcc")
  set(CMAKE_C_COMPILER   "gcc")
  set(CMAKE_CXX_COMPILER "g++")
  add_definitions(-Wno-strict-aliasing -fopenmp -Wall)

  if("${CMAKE_BUILD_TYPE}" STREQUAL "release")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "5.0")
      # When this option is on, compiler versions below 5.0 cause errors with
      # boost:
      #   undefined reference to
      #     `virtual thunk to boost::exception_detail::clone_impl<...'
      #   undefined reference to
      #     `boost::exception_detail::error_info_injector<...'
      add_definitions(-flto)
    endif()

    add_definitions(-funroll-loops -fomit-frame-pointer)

    # The following will omit all symbol information from the build:
    #add_definitions(-Wl,-s)
    #set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -s")
  endif()
elseif("${toolchain}" STREQUAL "intel")
  set(CMAKE_C_COMPILER   "icc")
  set(CMAKE_CXX_COMPILER "icpc")
  add_definitions(-openmp)

elseif("${toolchain}" STREQUAL "clang")
  set(CMAKE_C_COMPILER   "clang")
  set(CMAKE_CXX_COMPILER "clang++")
  add_definitions(-Wall)
else()
  message(FATAL_ERROR "Invalid toolchain: ${TOOLCHAIN}")
endif()

# Append "_d" to the library when doing debug build
if (CMAKE_BUILD_TYPE STREQUAL "debug")
  set(LIB_SUFFIX "_d")
endif()

# Note: explicit c++14 definitions done in CMakeInit.txt.
# Alternative is to set for each target:
# target_compile_features(${PROJECT_NAME} PRIVATE cxx_lambda_init_captures)

add_definitions(
	-D_REENTRANT
	-Wno-unused-local-typedefs
  -Wno-deprecated-declarations
	-DBOOST_SYSTEM_NO_DEPRECATED
)

message(STATUS "Configuring for the "
               "${BoldMagenta}${TOOLCHAIN}${ClrReset} toolchain version "
               "${CMAKE_CXX_COMPILER_VERSION} "
               "${BoldMagenta}${CMAKE_BUILD_TYPE}${ClrReset} build")

#-------------------------------------------------------------------------------
# Policies
#-------------------------------------------------------------------------------
# Don't curse at non-existing dependencies (since we use code generation)
cmake_policy(SET CMP0046 OLD)

# RPATH configuration
# ===================
# Don't skip the full RPATH for the build tree
set(CMAKE_SKIP_BUILD_RPATH  FALSE)
# When building, don't use the install RPATH already
# (but later on when installing)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "${CMAKE_BINARY_DIR}/src:${CMAKE_INSTALL_PREFIX}/lib")
# Add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

#-------------------------------------------------------------------------------
# Platform-specific checks
#-------------------------------------------------------------------------------
#include(${CMAKE_ROOT}/Modules/CheckTypeSize.cmake)
#include(${CMAKE_ROOT}/Modules/CheckFunctionExists.cmake)
include(${CMAKE_ROOT}/Modules/FindZLIB.cmake)
include(${CMAKE_ROOT}/Modules/CheckStructHasMember.cmake)
include(${CMAKE_ROOT}/Modules/CheckIncludeFile.cmake)

if(ZLIB_FOUND)
	set(UTXX_HAVE_LIBZ 1)
	list(APPEND UTXX_LIBS ${ZLIB_LIBRARIES})
endif()

execute_process(COMMAND getconf LEVEL1_DCACHE_LINESIZE OUTPUT_VARIABLE UTXX_CL_SIZE)

#check_type_size(size_t SIZE_T)

# Needed for Thrift
CHECK_INCLUDE_FILE(inttypes.h   HAVE_INTTYPES_H)
CHECK_INCLUDE_FILE(netinet/in.h HAVE_NETINET_IN_H)
# Needed for pcap.hpp tests
#CHECK_STRUCT_HAS_MEMBER("struct tcphdr" th_flags netinet/tcp.h UTXX_HAVE_TCPHDR_TH_FLAGS_H)

message("TCP: ${UTXX_HAVE_TCPHDR_TH_FLAGS_H}")
#-------------------------------------------------------------------------------
# Dependent packages and their directory locations
#-------------------------------------------------------------------------------
find_package(PkgConfig)

set(PKG_ROOT_DIR "/opt/pkg" CACHE STRING "Package root directory")

# Python
find_package(PythonInterp)
if(PYTHON_VERSION_STRING VERSION_LESS 2.7)
  message(FATAL_ERROR "Python version less than 2.y: \"${PYTHON_VERSION_STRING}\".")
endif()

if(WITH_THRIFT)
	if (NOT Thrift_DIR)
	  set(Thrift_DIR         "${PKG_ROOT_DIR}/thrift/current")
	endif()
  # Determine thrift include/libs
  set(ENV{PKG_CONFIG_PATH} "${Thrift_DIR}/gcc/lib/pkgconfig:${Thrift_DIR}/lib/pkgconfig")
  pkg_search_module(Thrift thrift)
	if (Thrift_FOUND)
		message(STATUS "Found thrift: ${Thrift_DIR}")
		set(UTXX_HAVE_THRIFT_H 1)
 	endif()
else()
  message(STATUS "Building without Thrift")
endif()

# Boost (with local modifications):
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREAD ON)
set(Boost_NO_SYSTEM_PATHS ON)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/build")

find_package(Boost 1.55.0 REQUIRED COMPONENTS
             system filesystem date_time program_options thread regex
             unit_test_framework timer)

if(Boost_FOUND)
  #include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
  #link_directories(${Boost_LIBRARY_DIRS})
  set(UTXX_HAVE_BOOST_TIMER_TIMER_HPP 1)
	message(STATUS "Found boost: ${Boost_LIBRARY_DIRS}")
endif()

set(Boost_LIBRARIES
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_THREAD_LIBRARY}
  ${Boost_REGEX_LIBRARY}
  ${Boost_DATE_TIME_LIBRARY}
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${Boost_IOSTREAMS_LIBRARY}
)

set(AddCleanFiles)  # Additional clean files

#===============================================================================
# Code generation
#===============================================================================
macro(XML_CFG TargetSrcList File)
  get_filename_component(FileName ${File} NAME)
  get_filename_component(BaseName ${File} NAME_WE) # Name without extension
  get_filename_component(DirName  ${File} DIRECTORY)
  if (DirName MATCHES "^/")
    set(SrcFile "${File}")
  else()
    if (DirName STREQUAL "")
      set(DirName "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()
    set(SrcFile "${DirName}/${File}")
  endif()
  string(REGEX REPLACE "^${CMAKE_SOURCE_DIR}/?" "" RelDirName ${DirName})
  string(REGEX REPLACE "^([^/].+)" "/\\1"          RelDirName ${RelDirName})
  set(GenFileName "${CMAKE_BINARY_DIR}${RelDirName}/generated/${BaseName}.generated.hpp")
  get_filename_component(GenDirName ${GenFileName} DIRECTORY)
  set(Deps ${ARGN})
  PREFIX(Deps "${CMAKE_CURRENT_SOURCE_DIR}/" "")
  message(STATUS "  >> ${SrcFile} -> ${GenFileName}")
  if (Deps STREQUAL "")
    unset(DepComment)
  else()
    set(DepComment "\n    Dependencies: ${Deps}")
  endif()
  set(GenIncDir ${CMAKE_CURRENT_SOURCE_DIR}:${CMAKE_SOURCE_DIR}/include:${CMAKE_BINARY_DIR}/include)
  file(MAKE_DIRECTORY "${GenDirName}")
  add_custom_command(
    OUTPUT            "${GenFileName}"
    COMMAND           "${CMAKE_SOURCE_DIR}/bin/config_validator_codegen.py"
                      -f "${SrcFile}" -o "${GenFileName}" --overwrite
                      -d ${GenIncDir}
    MAIN_DEPENDENCY   "${SrcFile}"
    # The following dependency may be extraneous due to some .hpp file
    # depending on a specific .xml file which "includes" the following file:
    DEPENDS           ${Deps}
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT           ">>> ${BoldBlue}Generating: ${FileName} -> ${GenFileName}${DepComment}${ClrReset}"
  )
  # TargetSrcList contains the list name and therefore ${TargetSrcList} is
  # the list itself, and ${${TargetSrcList}} is the expanded list
  set(${TargetSrcList} ${${TargetSrcList}} ${GenFileName})
endmacro(XML_CFG)

#-------------------------------------------------------------------------------
# MAKE options
#-------------------------------------------------------------------------------

#add_custom_target(vars
#  COMMAND ${CMAKE_COMMAND} -H${CMAKE_SOURCE_DIR} -B${CMAKE_BINARY_DIR} -LA
#)

include_directories(
  SYSTEM
  ${Boost_INCLUDE_DIRS}
  ${Thrift_INCLUDE_DIRS}
)
include_directories(
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_BINARY_DIR}/include
)
link_directories(
  ${Boost_LIBRARY_DIRS}
  ${Thrift_LIBRARY_DIRS}
)

#-------------------------------------------------------------------------------
# Configure files
#-------------------------------------------------------------------------------
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
               "${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/config.h"
 							 @ONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.pc.in"
               "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc" @ONLY)
#-------------------------------------------------------------------------------
# Srcs and Targets:
#-------------------------------------------------------------------------------
list(APPEND UTXX_LIBS ${Thrift_LIBRARIES} rt pthread)

add_subdirectory(src)
add_subdirectory(test)

# Copy configuration files to the build directory
#foreach(File etc/hs-replay.config etc/logger.config etc/hs-secdef.txt)
#  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${File}
#                 ${CMAKE_CURRENT_BINARY_DIR}/${File} COPYONLY)
#endforeach(File)

#===============================================================================
# Installation
#===============================================================================
install(
  DIRECTORY   ${CMAKE_SOURCE_DIR}/include/${PROJECT_NAME}
  DESTINATION ${CMAKE_INSTALL_PREFIX}/include
  FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.ipp" PATTERN "*.x??"
)
install(
  FILES       ${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/config.h
  DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}
)
install(
	PROGRAMS    ${CMAKE_SOURCE_DIR}/bin/config_validator_codegen.py
	DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)
install(
	FILES 		  ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc
  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig
)
install(
	FILES 		  ${CMAKE_SOURCE_DIR}/LICENSE
						  ${CMAKE_SOURCE_DIR}/README.md
	DESTINATION ${CMAKE_INSTALL_PREFIX}/share 
)

#===============================================================================
# Uninstallation
# Prereq: copy the uninstall.cmake file to the appropriate CMAKE_MODULE_PATH.
#===============================================================================
set_directory_properties(
  PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${AddCleanFiles}"
)

#add_custom_target(
#  uninstall "${CMAKE_COMMAND}" -P "${CMAKE_MODULE_PATH}/uninstall.cmake"
#)

#===============================================================================
# CTEST options
#===============================================================================
enable_testing()

add_test(test-utxx test/test_utxx -l message)

#===============================================================================
# Documentation options
#===============================================================================
# add a target to generate API documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/build-aux/Doxyfile.in
                 ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
                 @ONLY)
  add_custom_target(doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "${ClrBold}Generating API documentation with Doxygen${ClrReset}"
    VERBATIM
  )
endif()

# Post-install script (installation of symlinks):
install(SCRIPT ${CMAKE_SOURCE_DIR}/build-aux/install-symlinks.cmake)
