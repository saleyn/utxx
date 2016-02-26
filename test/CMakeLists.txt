# vim:ts=2:sw=2:et

list(APPEND TEST_SRCS
    test_alloc_fixed_page.cpp
    test_atomic_hash_array.cpp
    test_atomic_hash_map.cpp
    test_assoc_vector.cpp
    test_async_file_logger.cpp
    test_basic_udp_receiver.cpp
    test_buffer.cpp
    test_call_speed.cpp
    test_clustered_map.cpp
    test_compiler_hints.cpp
    test_collections.cpp
    test_concurrent_stack.cpp
    test_concurrent_update.cpp
    test_concurrent_spsc_queue.cpp
    test_concurrent_mpsc_queue.cpp
    test_config_validator.cpp
    test_convert.cpp
    test_enum.cpp
    test_error.cpp
    test_file_reader.cpp
    test_futex.cpp
    test_function.cpp
    test_get_option.cpp
    test_gzstream.cpp
    test_hashmap.cpp
    test_high_res_timer.cpp
    test_iovec.cpp
    test_iovector.cpp
    test_leb128.cpp
    test_logger.cpp
    test_logger_scribe.cpp
    test_logger_syslog.cpp
    test_math.cpp
    test_meta.cpp
    test_multi_file_async_logger.cpp
    test_nchar.cpp
    test_os.cpp
    test_path.cpp
    test_pcap.cpp
    test_pmap.cpp
    test_print.cpp
    test_persist_array.cpp
    test_persist_blob.cpp
    test_pidfile.cpp
    test_rate_throttler.cpp
    test_registrar.cpp
    test_robust_mutex.cpp
    test_ring_buffer.cpp
    test_running_stat.cpp
    test_scope_exit.cpp
    test_stream_io.cpp
    test_shared_queue.cpp
    test_shared_ptr.cpp
    test_signal_block.cpp
    test_stack_container.cpp
    test_static_polymorphism.cpp
    test_string.cpp
    test_thread_cached_int.cpp
    test_thread_local.cpp
    test_time_val.cpp
    test_timestamp.cpp
    test_type_traits.cpp
    test_url.cpp
    test_utxx.cpp
    test_variant.cpp
    test_variant_tree_scon_parser.cpp
    test_verbosity.cpp
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(STATUS "Including tests for x64 platform")
  list(APPEND TEST_SRCS
    test_atomic.cpp
    test_bitmap.cpp
    test_concurrent_fifo.cpp
    test_name.cpp
    test_stream64.cpp
  )
endif()

XML_CFG(TEST_SRCS test_config_validator.xml)
XML_CFG(TEST_SRCS test_config_validator2.xml)

# This must be AFTER link_directories
add_executable(test_utxx ${TEST_SRCS})
target_compile_definitions(test_utxx PRIVATE -DBOOST_TEST_DYN_LINK)
target_include_directories(test_utxx PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(
  test_utxx
  utxx
  boost_unit_test_framework
  boost_timer
)

install(TARGETS test_utxx RUNTIME DESTINATION test)

add_executable(fast_read example_fast_read.cpp)

add_executable(example_repeating_timer example_repeating_timer.cpp)
target_link_libraries(example_repeating_timer utxx ${Boost_LIBRARIES} boost_timer)

add_executable(example_delegate example_delegate.cpp)
target_link_libraries(example_delegate utxx ${Boost_LIBRARIES} boost_timer)

install(
  TARGETS fast_read example_repeating_timer example_delegate
  RUNTIME DESTINATION test
)
