util_LTLIBRARIES = libutil.la

util_headers = \
		allocator.hpp \
		alloc_cached.hpp \
		alloc_fixed_page.hpp \
		alloc_fixed_pool.hpp \
		async_file_logger.hpp \
		atomic.hpp \
		bitmap.hpp \
		bits.hpp \
		boost/repeating_timer.hpp \
		buffer.hpp \
		compiler_hints.hpp \
		concurrent_alloc_fixed_page.hpp \
		config_validator.hpp \
		config_validator.xsl \
		container/concurrent_array.hpp \
		container/concurrent_fifo.hpp \
		container/concurrent_priority_queue.hpp \
		container/concurrent_stack.hpp \
		container/detail/base_allocator.hpp \
		container/detail/base_fifo.hpp \
		convert.hpp \
		cpu.hpp \
		delegate/delegate.hpp \
		delegate/detail/delegate_list.hpp \
		delegate/detail/delegate_template.hpp \
		delegate/detail/make_delegates.pl \
		delegate/event.hpp \
		delegate/FastDelegateBind.hpp \
		delegate/FastDelegate.hpp \
		delegate/flex_binder.hpp \
		delegate.hpp \
		delegate/LICENSE \
		detail/bit_count.hpp \
		detail/cpu_x86.hpp \
		detail/get_tick_count.hpp \
		detail/gettimeofday.hpp \
		detail/handler_traits.hpp \
		detail/mean_variance.hpp \
		endian.hpp \
		epoll.hpp \
		error.hpp \
		event.hpp \
		hashmap.hpp \
		high_res_timer.hpp \
		iovec.hpp \
		iovector.hpp \
		logger.hpp \
		logger/logger.hpp \
		logger/logger_impl_async_file.hpp \
		logger/logger_impl_console.hpp \
		logger/logger_impl_file.hpp \
		logger/logger_impl.hpp \
		logger/logger_impl.ipp \
		logger/logger_impl_syslog.hpp \
		math.hpp \
		meta.hpp \
		mt_queue.hpp \
		multi_file_async_logger.hpp \
		name.hpp \
		nchar.hpp \
		path.hpp \
		pcap.hpp \
		perf_histogram.hpp \
		persist_blob.hpp \
		persist_array.hpp \
		pidfile.hpp \
		rate_throttler.hpp \
		robust_mutex.hpp \
		running_stat.hpp \
		signal_block.hpp \
		singleton.hpp \
		string.hpp \
		synch.hpp \
		test_helper.hpp \
		timestamp.hpp \
		time_val.hpp \
		typeinfo.hpp \
		url.hpp \
		variant.hpp \
		variant_tree.hpp \
		verbosity.hpp

libutil_la_SOURCES = \
		config_validator.cpp \
		convert.cpp \
		high_res_timer.cpp \
		logger.cpp \
		logger_impl_async_file.cpp \
		logger_impl_console.cpp \
		logger_impl.cpp \
		logger_impl_file.cpp \
		logger_impl_syslog.cpp \
		name.cpp \
		path.cpp \
		synch.cpp \
		timestamp.cpp \
		url.cpp

libutil_la_LTADD    = -version-info 1:0:0 -shared
libutil_la_LIBS     = $(BOOST_THREAD_LIBS)
libutil_la_CXXFLAGS = -I../include $(BOOST_CPPFLAGS)

nobase_include_HEADERS = $(util_headers:%=../include/@PACKAGE@/%)

nodist_libutil_la_SOURCES = ../config.h

EXTRA_DIST = config_validator.xsl
