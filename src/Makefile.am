utxx_LTLIBRARIES = libutxx.la

bin_PROGRAMS = mreceive tailagg

UTXX64_SRCS = \
    name.cpp

UTXX_SRCS = \
    config_validator.cpp \
    error.cpp \
    futex.cpp \
    high_res_timer.cpp \
    logger.cpp \
    logger_crash_handler.cpp \
    logger_impl.cpp \
    logger_impl_console.cpp \
    logger_impl_file.cpp \
    logger_impl_scribe.cpp \
    logger_impl_syslog.cpp \
    path.cpp \
    stream64.cpp \
    string.cpp \
    timestamp.cpp \
    url.cpp \
    verbosity.cpp

THRIFT 				= 
if Ix86
libutxx_la_SOURCES  = $(UTXX_SRCS)
else
libutxx_la_SOURCES  = $(UTXX64_SRCS) $(UTXX_SRCS)
endif
libutxx_la_LDFLAGS  = -version-info 1:1:0 -shared \
                      -rpath $(BOOST_LIBDIR) \
                      $(if $(THRIFT_LIBDIR),-rpath $(THRIFT_LIBDIR)) \
                      $(BOOST_LDFLAGS) $(THRIFT_LDFLAGS)
libutxx_la_LIBS     = $(BOOST_THREAD_LIB) $(THRIFT_LIBS)
libutxx_la_CXXFLAGS = -I${top_srcdir}/include $(THRIFT_CPPFLAGS) \
                      -I${top_srcdir}/src -I$(THRIFT_DIR)/include/thrift/fb303

mreceive_SOURCES    = mreceive.c
mreceive_CFLAGS     = -g

tailagg_SOURCES     = tailagg.cpp
tailagg_CPPFLAGS    = -g -O3 -I${top_srcdir}/include
tailagg_LDFLAGS     = -rpath $(libdir) -L$(srcdir)/../src/.libs $(BOOST_LDFLAGS)
tailagg_LDADD       = libutxx.la $(BOOST_SYSTEM_LIB) $(BOOST_FILESYSTEM_LIB) \
                      $(THRIFT_LIBS)

bin_SCRIPTS = ${top_srcdir}/bin/config_validator_codegen.py

nobase_include_HEADERS = \
            $(sort ${top_srcdir}/include/@PACKAGE@/config_validator.xsl \
            ${top_srcdir}/include/@PACKAGE@/logger/logger_options.xml \
            $(foreach d, \
                $(shell find ${top_srcdir}/include/@PACKAGE@ -type d -print | xargs), \
            $(wildcard $(d)/*.?pp)) \
            ${top_srcdir}/include/@PACKAGE@/config.h)

nodist_libutxx_la_SOURCES = ${top_srcdir}/config.h

logger.cpp: ${top_srcdir}/include/@PACKAGE@/logger/logger_options.generated.hpp

${top_srcdir}/include/@PACKAGE@/logger/logger_options.generated.hpp: \
	    ${top_srcdir}/include/@PACKAGE@/logger/logger_options.xml \
	    $(top_srcdir)/bin/config_validator_codegen.py
	$(PYTHON) $(top_srcdir)/bin/config_validator_codegen.py -f $< -o $@ --overwrite

EXTRA_DIST = config_validator.xsl
