# vim:ts=2:sw=2:et

string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE)

list(APPEND UTXX_SRCS
  config_validator.cpp
  error.cpp
  futex.cpp
  gzstream.cpp
  high_res_timer.cpp
  logger.cpp
  logger_crash_handler.cpp
  logger_impl.cpp
  logger_impl_console.cpp
  logger_impl_file.cpp
  logger_impl_scribe.cpp
  logger_impl_syslog.cpp
  path.cpp
  signal_block.cpp
  string.cpp
  timestamp.cpp
  url.cpp
  variant.cpp
  verbosity.cpp
)

XML_CFG(UTXX_SRCS ${CMAKE_SOURCE_DIR}/include/${PROJECT_NAME}/logger/logger_options.xml)

# this is the "object library" target: compiles the sources only once
#add_library(objlib OBJECT ${UTXX_SRCS})
# shared libraries need PIC
#set_target_properties(${objlib} PROPERTIES POSITION_INDEPENDENT_CODE 1)
#set_property(TARGET ${objlib} PROPERTY POSITION_INDEPENDENT_CODE 1)
# actual shared and static libraries built from the same object files
#add_library(${PROJECT_NAME} SHARED $<TARGET_OBJECTS:objlib>)
#add_library(${PROJECT_NAME}_static STATIC $<TARGET_OBJECTS:objlib>)
add_library(${PROJECT_NAME} SHARED ${UTXX_SRCS})
add_library(${PROJECT_NAME}_static STATIC ${UTXX_SRCS})
set_target_properties(${PROJECT_NAME}_static PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} ${UTXX_LIBS})
if(ZLIB_FOUND)
  target_link_libraries(${PROJECT_NAME} ${LIBZ_LIBRARIES})
endif()

if (${CMAKE_BUILD_TYPE} STREQUAL "debug")
  set_target_properties(${PROJECT_NAME} PROPERTIES
    DEBUG_POSTFIX   "${LIB_SUFFIX}"
    RELEASE_POSTFIX "")
  set_target_properties(${PROJECT_NAME}_static PROPERTIES
    DEBUG_POSTFIX   "${LIB_SUFFIX}"
    RELEASE_POSTFIX "")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})

add_executable(mreceive  mreceive.cpp)
set_source_files_properties(mreceive.c PROPERTIES COMPILE_FLAGS -Wno-effc++)

add_executable(tailagg   tailagg.cpp)
target_link_libraries(tailagg utxx)
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})

# In the install below we split library installation in a separate library clause
# so that it's possible to build/install both Release and Debug versions of the
# library and then include that into a package

install(
  TARGETS ${PROJECT_NAME} ${PROJECT_NAME}_static mreceive tailagg
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

if (${CMAKE_BUILD_TYPE} STREQUAL "release")
  install(
    FILES ${CMAKE_BINARY_DIR}/src/lib${PROJECT_NAME}_d.so.${PROJECT_VERSION}
    DESTINATION lib
  )
  install(
    FILES ${CMAKE_BINARY_DIR}/src/lib${PROJECT_NAME}_d.so
    DESTINATION lib
  )
  install(
    FILES ${CMAKE_BINARY_DIR}/src/lib${PROJECT_NAME}_d.a
    DESTINATION lib
  )
endif()

install(
  FILES ${CMAKE_BINARY_DIR}/include/${PROJECT_NAME}/logger/generated/logger_options.generated.hpp
  DESTINATION ${CMAKE_INSTALL_PREFIX}/include/logger
)

