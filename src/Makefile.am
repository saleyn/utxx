utxx_LTLIBRARIES = libutxx.la

UTXX64_SRCS = \
	name.cpp

UTXX_SRCS = \
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
	stream64.cpp \
	synch.cpp \
	timestamp.cpp \
	url.cpp

if Ix86
libutxx_la_SOURCES  = $(UTXX_SRCS)
else
libutxx_la_SOURCES  = $(UTXX64_SRCS) $(UTXX_SRCS)
endif
libutxx_la_LTADD    = -version-info 1:0:0 -shared
libutxx_la_LIBS     = $(BOOST_LDFLAGS) $(BOOST_THREAD_LIB)
libutxx_la_CXXFLAGS = -I../include $(BOOST_CPPFLAGS)

nobase_include_HEADERS = \
	$(sort ../include/@PACKAGE@/config_validator.xsl \
		$(foreach d, \
			$(shell find ../include/@PACKAGE@ -type d -print),\
		  	$(wildcard $(d)/*.?pp)))

nodist_libutxx_la_SOURCES = ../config.h

EXTRA_DIST = config_validator.xsl
