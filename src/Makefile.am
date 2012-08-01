util_LTLIBRARIES = libutil.la

if Ix86
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
	path.cpp \
	synch.cpp \
	timestamp.cpp \
	url.cpp
else
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
endif

libutil_la_LTADD    = -version-info 1:0:0 -shared
libutil_la_LIBS     = $(BOOST_LDFLAGS) $(BOOST_THREAD_LIB)
libutil_la_CXXFLAGS = -I../include $(BOOST_CPPFLAGS)

nobase_include_HEADERS = \
	$(sort ../include/@PACKAGE@/config_validator.xsl \
		$(foreach d, \
			$(shell find ../include/@PACKAGE@ -type d -print),\
		  	$(wildcard $(d)/*.?pp)))

nodist_libutil_la_SOURCES = ../config.h

EXTRA_DIST = config_validator.xsl
