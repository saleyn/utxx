# vim:ts=2:sw=2:et

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

add_library(${PROJECT_NAME} SHARED ${UTXX_SRCS})
target_link_libraries(${PROJECT_NAME} ${UTXX_LIBS})
if(ZLIB_FOUND)
  target_link_libraries(${PROJECT_NAME} ${LIBZ_LIBRARIES})
endif()

if (${BUILD_TYPE} STREQUAL "Debug")
  set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX "-d.so")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})

add_executable(mreceive  mreceive.cpp)
set_source_files_properties(mreceive.c PROPERTIES COMPILE_FLAGS -Wno-effc++)

add_executable(tailagg   tailagg.cpp)
target_link_libraries(tailagg utxx)
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})

install(
  TARGETS ${PROJECT_NAME} mreceive tailagg
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
)
