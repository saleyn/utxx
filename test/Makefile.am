bindir			   	= ${exec_prefix}/test
#datadir			 	= ${exec_prefix}/test
test_utxxdir        = ${exec_prefix}/test

bin_PROGRAMS       	= test_utxx \
					  example_delegate \
			  		  example_repeating_timer \
					  example_scribe \
                      example_fast_read

TEST_UTXX64_SRCS = \
	test_atomic.cpp \
	test_bitmap.cpp \
	test_concurrent_fifo.cpp \
	test_name.cpp

TEST_UTXX_SRCS = \
	test_alloc_fixed_page.cpp \
	test_async_file_logger.cpp \
	test_basic_udp_receiver.cpp \
	test_buffer.cpp \
	test_clustered_map.cpp \
	test_collections.cpp \
	test_concurrent_stack.cpp \
	test_concurrent_update.cpp \
	test_concurrent_spsc_queue.cpp \
	test_concurrent_mpsc_queue.cpp \
	test_config_validator.cpp \
	test_convert.cpp \
	test_enum.cpp \
	test_error.cpp \
	test_file_reader.cpp \
	test_futex.cpp \
	test_function.cpp \
	test_hashmap.cpp \
	test_high_res_timer.cpp \
	test_iovec.cpp \
	test_iovector.cpp \
	test_logger.cpp \
	test_logger_async_file.cpp \
	test_logger_scribe.cpp \
	test_logger_syslog.cpp \
	test_math.cpp \
	test_meta.cpp \
	test_multi_file_async_logger.cpp \
	test_nchar.cpp \
	test_path.cpp \
	test_pcap.cpp \
	test_pmap.cpp \
	test_print.cpp \
	test_persist_array.cpp \
	test_persist_blob.cpp \
	test_pidfile.cpp \
	test_registrar.cpp \
	test_robust_mutex.cpp \
	test_ring_buffer.cpp \
	test_running_stat.cpp \
	test_scope_exit.cpp \
	test_shared_queue.cpp \
	test_shared_ptr.cpp \
	test_static_polymorphism.cpp \
	test_stream64.cpp \
	test_string.cpp \
	test_throttler.cpp \
	test_time_val.cpp \
	test_timestamp.cpp \
	test_type_traits.cpp \
	test_url.cpp \
	test_utxx.cpp \
	test_variant.cpp \
	test_variant_tree_scon_parser.cpp \
	test_verbosity.cpp

example_repeating_timer_SOURCES = example_repeating_timer.cpp
example_fast_read_SOURCES       = example_fast_read.cpp


generated/test_config_validator.generated.hpp: \
	   	test_config_validator.xml $(top_srcdir)/bin/config_validator_codegen.py \
		$(top_srcdir)/test/generated
	$(PYTHON) $(top_srcdir)/bin/config_validator_codegen.py -f $< -o $@ --overwrite

# XSLT transform is deprecated
#generated/test_config_validator.generated.hpp: test_config_validator.xml \
#		$(srcdir)/../include/@PACKAGE@/config_validator.xsl generated
#	$(XSLTPROC) --stringparam user \
#		"$(shell awk -F: '$$1 == u { print $$5 }' u=$(USER) /etc/passwd)" \
#		--stringparam file "$(notdir $@)" \
#		--stringparam now "$(shell date +'%Y-%m-%d')" $(word 2,$^) $< > $@

test_config_validator.cpp: generated/test_config_validator.generated.hpp

$(top_srcdir)/test/generated:
	mkdir -v $@

AM_CPPFLAGS = $(if $(debug),\
				-g $(if $(optimize),-O$(optimize),-O0),\
				$(if $(optimize),-O$(optimize) -DBOOST_DISABLE_ASSERTS))

if Ix86
test_utxx_SOURCES  	= $(TEST_UTXX_SRCS)
else
test_utxx_SOURCES  	= $(TEST_UTXX64_SRCS) $(TEST_UTXX_SRCS)
endif

test_utxx_CXXFLAGS 	= -I$(top_srcdir)/include -DBOOST_TEST_DYN_LINK $(THRIFT_CPPFLAGS)
test_utxx_LDFLAGS  	= $(BOOST_LDFLAGS) \
                      -L$(srcdir)/../src/.libs -lrt \
                      -rpath $(abspath $(srcdir)/../src/.libs) \
                      -rpath $(abspath $(prefix)/gcc/lib) \
                      $(if $(THRIFT_LIBDIR),-rpath $(THRIFT_LIBDIR))

test_utxx_LDADD    	= -lutxx \
                          $(BOOST_SYSTEM_LIB) $(BOOST_THREAD_LIB) \
                          $(BOOST_DATE_TIME_LIB) \
                          $(BOOST_TIMER_LIB) \
                          $(BOOST_FILESYSTEM_LIB) \
                          $(BOOST_UNIT_TEST_FRAMEWORK_LIB) \
                          $(THRIFT_LDFLAGS) $(THRIFT_LIBS) \
                          $(RTLIB)

test_utxx_DATA 		= test_config_validator.xml


example_repeating_timer_CXXFLAGS = -I$(top_srcdir)/include
example_repeating_timer_LDFLAGS  = $(BOOST_LDFLAGS)
example_repeating_timer_LDADD    = $(srcdir)/../src/.libs/libutxx.la \
                                   $(BOOST_SYSTEM_LIB) $(BOOST_THREAD_LIB) \
                                   $(BOOST_DATE_TIME_LIB) $(BOOST_CHRONO_LIB) \
                                   $(BOOST_TIMER_LIB) $(BOOST_FILESYSTEM_LIB) \
                                   $(THRIFT_LDFLAGS) $(THRIFT_LIBS)

example_delegate_SOURCES	= example_delegate.cpp
example_delegate_CXXFLAGS 	= $(if $(debug),\
                                -g $(if $(optimize),-O$(optimize),-O0),\
                                $(if $(optimize),-O$(optimize))) \
					          -DUTXX_STANDALONE -I$(srcdir)/../include
example_delegate_LDFLAGS        = $(BOOST_LDFLAGS)
example_delegate_LDADD          = -lrt $(BOOST_SYSTEM_LIB) $(BOOST_TIMER_LIB) \
                                  $(THRIFT_LDFLAGS) $(THRIFT_LIBS)

example_scribe_SOURCES          = test_logger_scribe.cpp
example_scribe_CXXFLAGS         = -DUTXX_STANDALONE -I$(srcdir)/../include $(THRIFT_CPPFLAGS) \
							  -I${top_srcdir}/src -I$(THRIFT_DIR)/include/thrift/fb303
example_scribe_LDFLAGS          = $(BOOST_SYSTEM_LIB) $(BOOST_TIMER_LIB) $(THRIFT_LDFLAGS)
example_scribe_LDADD            = $(BOOST_SYSTEM_LIB) $(BOOST_FILESYSTEM_LIB) \
                                  $(BOOST_THREAD_LIB) $(THRIFT_LIBS) \
                                  $(srcdir)/../src/.libs/libutxx.la

EXTRA_DIST = test_config_validator.xml $(top_src_dir)/bin/config_validator_codegen.py
