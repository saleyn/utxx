util_LTLIBRARIES = libutil.la

util_headers = epoll.hpp nchar.hpp config_validator.hpp alloc_fixed_pool.hpp \
			   typeinfo.hpp math.hpp timestamp.hpp delegate/flex_binder.hpp \
			   delegate/event.hpp delegate/detail/delegate_template.hpp \
			   delegate/detail/delegate_list.hpp delegate/FastDelegateBind.hpp \
			   delegate/FastDelegate.hpp delegate/delegate.hpp verbosity.hpp \
			   path.hpp hashmap.hpp time_val.hpp alloc_fixed_page.hpp \
			   event.hpp convert.hpp variant_tree.hpp detail/get_tick_count.hpp \
			   detail/gettimeofday.hpp detail/bit_count.hpp \
			   detail/handler_traits.hpp detail/cpu_x86.hpp \
			   detail/mean_variance.hpp pidfile.hpp iovector.hpp synch.hpp \
			   test_helper.hpp cpu.hpp logger.hpp allocator.hpp alloc_cached.hpp \
			   container/detail/base_fifo.hpp container/detail/base_allocator.hpp \
			   container/concurrent_array.hpp container/concurrent_fifo.hpp \
			   container/concurrent_priority_queue.hpp \
			   container/concurrent_stack.hpp url.hpp high_res_timer.hpp \
			   singleton.hpp bits.hpp buffer.hpp variant.hpp bitmap.hpp \
			   mt_queue.hpp running_stat.hpp atomic.hpp error.hpp endian.hpp \
			   meta.hpp signal_block.hpp logger/logger.hpp logger/logger_impl.hpp \
			   logger/logger_impl_syslog.hpp logger/logger_impl_file.hpp \
			   logger/logger_impl.ipp logger/logger_impl_console.hpp \
			   logger/logger_impl_async_file.hpp string.hpp iovec.hpp \
			   concurrent_alloc_fixed_page.hpp rate_throttler.hpp \
			   compiler_hints.hpp pcap.hpp delegate.hpp

libutil_la_SOURCES = \
	timestamp.cpp \
	path.cpp \
	high_res_timer.cpp \
	logger.cpp \
	logger_impl.cpp \
	logger_impl_console.cpp \
	logger_impl_async_file.cpp \
	logger_impl_syslog.cpp \
	synch.cpp \
	convert.cpp \
	url.cpp \
	config_validator.cpp \
	logger_impl_file.cpp
libutil_la_LTADD    = -version-info 1:0:0 -shared
libutil_la_LIBS     = $(BOOST_THREAD_LIBS)
libutil_la_CXXFLAGS = -I../include $(BOOST_CPPFLAGS)
#libutil_la_LDFLAGS  = -shared

#libutil_la_includedir=$(includedir)/@PACKAGE@-@PACKAGE_VERSION@/@PACKAGE@
libutil_la_includedir=$(includedir)/@PACKAGE@
libutil_la_include_HEADERS = $(util_headers:%=../include/@PACKAGE@/%)

#If you have multiple architectures, install config.h in architecture-specific location:
libutil_la_libincludedir = $(libdir)/@PACKAGE@
nodist_libutil_la_libinclude_HEADERS = ../config.h

EXTRA_DIST = config_validator.xsl
