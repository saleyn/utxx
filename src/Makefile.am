util_LTLIBRARIES = libutil.la

libutil_la_SOURCES = $(wildcard *.cpp)

libutil_la_LTADD    = -version-info 1:0:0 -shared
libutil_la_LIBS     = $(BOOST_LDFLAGS) $(BOOST_THREAD_LIB)
libutil_la_CXXFLAGS = -I../include $(BOOST_CPPFLAGS)

nobase_include_HEADERS = \
	$(sort $(wildcard ../include/@PACKAGE@/*.?pp) \
	   	$(foreach d, boost container delegate detail logger,\
		  	  $(wildcard ../include/@PACKAGE@/$(d)/*.?pp)))

nodist_libutil_la_SOURCES = ../config.h

EXTRA_DIST = config_validator.xsl
