utxx_LTLIBRARIES = libutxx.la

bin_PROGRAMS = mreceive

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
libutxx_la_CXXFLAGS = -I${top_srcdir}/include $(BOOST_CPPFLAGS)

mreceive_SOURCES    = mreceive.c
mreceive_CFLAGS     = -g

nobase_include_HEADERS = \
	$(sort ${top_srcdir}/include/@PACKAGE@/config_validator.xsl \
		   ${top_srcdir}/bin/config_validator_codegen.py \
		$(foreach d, \
			$(shell find ${top_srcdir}/include/@PACKAGE@ -type d -print0),\
		  	$(wildcard $(d)/*.?pp)))

nodist_libutxx_la_SOURCES = ${top_srcdir}/config.h

EXTRA_DIST = config_validator.xsl
