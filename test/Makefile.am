bindir			   	= ${exec_prefix}/test
#datadir			 	= ${exec_prefix}/test
test_utildir        = ${exec_prefix}/test

if Ix86
bin_PROGRAMS       	= test_util
else
bin_PROGRAMS       	= test_util \
			  example_repeating_timer
endif

if Ix86
test_util_SOURCES  	= \
	test_alloc_fixed_page.cpp \
	test_basic_udp_receiver.cpp \
	test_buffer.cpp \
	test_shared_queue.cpp \
	test_file_reader.cpp \
	test_collections.cpp \
	test_concurrent_stack.cpp \
	test_concurrent_update.cpp \
	test_config_validator.cpp \
	test_high_res_timer.cpp \
	test_iovec.cpp \
	test_iovector.cpp \
	test_logger_async_file.cpp \
	test_logger.cpp \
	test_logger_syslog.cpp \
	test_math.cpp \
	test_nchar.cpp \
	test_path.cpp \
	test_pcap.cpp \
	test_persist_array.cpp \
	test_pidfile.cpp \
	test_running_stat.cpp \
	test_string.cpp \
	test_throttler.cpp \
	test_url.cpp \
	test_util.cpp \
	test_variant.cpp
else
test_util_SOURCES  	= \
	test_alloc_fixed_page.cpp \
	test_atomic.cpp \
	test_basic_udp_receiver.cpp \
	test_bitmap.cpp \
	test_buffer.cpp \
	test_shared_queue.cpp \
	test_file_reader.cpp \
	test_collections.cpp \
	test_concurrent_fifo.cpp \
	test_concurrent_stack.cpp \
	test_concurrent_update.cpp \
	test_config_validator.cpp \
	test_convert.cpp \
	test_async_file_logger.cpp \
	test_high_res_timer.cpp \
	test_iovec.cpp \
	test_iovector.cpp \
	test_logger_async_file.cpp \
	test_logger.cpp \
	test_logger_syslog.cpp \
	test_math.cpp \
	test_multi_file_async_logger.cpp \
	test_name.cpp \
	test_nchar.cpp \
	test_path.cpp \
	test_pcap.cpp \
	test_persist_blob.cpp \
	test_persist_array.cpp \
	test_pidfile.cpp \
	test_robust_mutex.cpp \
	test_running_stat.cpp \
	test_string.cpp \
	test_throttler.cpp \
	test_timestamp.cpp \
	test_url.cpp \
	test_util.cpp \
	test_variant.cpp
endif

if Ix86
else
example_repeating_timer_SOURCES = \
	example_repeating_timer.cpp
endif

generated/test_config_validator.generated.hpp: test_config_validator.xml \
		$(srcdir)/../include/@PACKAGE@/config_validator.xsl generated
	$(XSLTPROC) --stringparam user \
		"$(shell awk -F: '$$1 == u { print $$5 }' u=$(USER) /etc/passwd)" \
		--stringparam file "$(notdir $@)" \
		--stringparam now "$(shell date +'%Y-%m-%d')" $(word 2,$^) $< > $@

test_config_validator.cpp: generated/test_config_validator.generated.hpp

generated:                                                                           
	mkdir $@

test_util_CPPFLAGS 	= -I$(srcdir)/../include -DBOOST_TEST_DYN_LINK \
					  $(BOOST_CPPFLAGS)
test_util_LDFLAGS  	= $(BOOST_LDFLAGS)

test_util_LDADD    	= $(srcdir)/../src/.libs/libutil.la \
					$(BOOST_SYSTEM_LIB) $(BOOST_THREAD_LIB) \
					$(BOOST_DATE_TIME_LIB) \
					$(BOOST_TIMER_LIB) \
					$(BOOST_CHRONO_LIB) \
					$(BOOST_FILESYSTEM_LIB) \
					$(BOOST_UNIT_TEST_FRAMEWORK_LIB)

test_util_DATA 		= test_config_validator.xml


example_repeating_timer_CPPFLAGS = -I$(srcdir)/../include $(BOOST_CPPFLAGS)
example_repeating_timer_LDFLAGS  = $(BOOST_LDFLAGS)
example_repeating_timer_LDADD    = $(srcdir)/../src/.libs/libutil.la \
					   $(BOOST_SYSTEM_LIB) $(BOOST_THREAD_LIB) \
					   $(BOOST_DATE_TIME_LIB) $(BOOST_CHRONO_LIB)

EXTRA_DIST = test_config_validator.xml
