bindir			   	= ${exec_prefix}/test
#datadir			 	= ${exec_prefix}/test
test_utildir        = ${exec_prefix}/test

bin_PROGRAMS       	= test_util

test_util_SOURCES  	= test_alloc_fixed_page.cpp \
					  test_atomic.cpp \
					  test_bitmap.cpp \
					  test_buffer.cpp \
					  test_concurrent_fifo.cpp \
					  test_concurrent_stack.cpp \
					  test_concurrent_update.cpp \
					  test_config_validator.cpp \
					  test_convert.cpp \
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
					  test_pidfile.cpp \
					  test_running_stat.cpp \
					  test_string.cpp \
					  test_throttler.cpp \
					  test_timestamp.cpp \
					  test_url.cpp \
					  test_util.cpp \
					  test_variant.cpp

generated/test_config_validator.generated.hpp: test_config_validator.xml \
		$(srcdir)/../src/config_validator.xsl generated
	$(XSLTPROC) --stringparam author \
		"$(shell awk -F: '$$1 == u { print $$5 }' u=$(USER) /etc/passwd)" \
		--stringparam file "$(notdir $@)" \
		--stringparam now "$(shell date +'%Y-%m-%d')" $(word 2,$^) $< > $@

test_config_validator.cpp: generated/test_config_validator.generated.hpp

generated:                                                                           
	mkdir $@

test_util_CPPFLAGS 	= -I$(srcdir)/../include -DBOOST_TEST_DYN_LINK \
					  $(BOOST_CPPFLAGS)
test_util_LDFLAGS  	= $(BOOST_SYSTEM_LDFLAGS) $(BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS)
test_util_LDADD    	= $(srcdir)/../src/.libs/libutil.la \
					  $(BOOST_SYSTEM_LIBS) $(BOOST_THREAD_LIBS) \
					  $(BOOST_UNIT_TEST_FRAMEWORK_LIBS) -lrt
test_util_DATA 		= test_config_validator.xml

EXTRA_DIST = test_config_validator.xml
