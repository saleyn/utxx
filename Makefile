# vim:ts=4:sw=4:et
#-------------------------------------------------------------------------------
# Makefile helper for cmake
#-------------------------------------------------------------------------------
# Copyright (c) 2015 Serge Aleynikov
# Date: 2014-08-12
#-------------------------------------------------------------------------------

-include build/cache.mk

VERBOSE := $(if $(findstring $(verbose),true 1),$(if $(findstring $(generator),ninja),-v,VERBOSE=1))

.DEFAULT_GOAL := all

distclean:
	@[ -n "$(DIR)" -a -d "$(DIR)" ] && echo "Removing $(DIR)" && rm -fr $(DIR) build inst || true

bootstrap:
	@$(MAKE) -f bootstrap.mk --no-print-directory $@ $(MAKEOVERRIDES)

info:
	@$(MAKE) -sf bootstrap.mk $@

test:
	CTEST_OUTPUT_ON_FAILURE=TRUE $(generator) -C$(DIR) $(VERBOSE) -j$(shell nproc) $@

build/cache.mk:

.DEFAULT:
	@if [ ! -f build/cache.mk ]; then \
	    $(MAKE) -f bootstrap.mk --no-print-directory; \
    else \
        $(generator) -C$(DIR) $(VERBOSE)\
            $(if $(findstring $(generator),ninja),, --no-print-directory)\
            -j$(shell nproc) $@;\
	fi

.PHONY: bootstrap distclean info test doc
