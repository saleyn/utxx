# vim:ts=4:sw=4:et
#-------------------------------------------------------------------------------
# Makefile helper for cmake
#-------------------------------------------------------------------------------
# Copyright (c) 2015 Serge Aleynikov
# Date: 2014-08-12
#-------------------------------------------------------------------------------

build          ?= debug
BUILD_ARG      := $(shell echo $(build) | tr 'A-Z' 'a-z')
REBOOTSTR_FILE := .build/.bootstrap

-include build/cache.mk

VERBOSE := $(if $(findstring $(verbose),true 1),$(if $(findstring $(generator),ninja),-v,VERBOSE=1))

.DEFAULT_GOAL := all

distclean:
	@[ -n "$(DIR)" -a -d "$(DIR)" ] && echo "Removing $(DIR)" && rm -fr $(DIR) build inst || true

bootstrap:
	@$(MAKE) -f build-aux/bootstrap.mk --no-print-directory $@ $(MAKEOVERRIDES)

rebootstrap: $(REBOOTSTR_FILE)
	$(if $(build),$(filter-out build=%,$(shell cat $(REBOOTSTR_FILE))) \
        build=$(BUILD_ARG),$(shell cat $(REBOOTSTR_FILE))) --no-print-directory $(MAKEOVERRIDES)

test:
	CTEST_OUTPUT_ON_FAILURE=TRUE $(generator) -C$(DIR) $(VERBOSE) -j$(shell nproc) $@

info ver:
	@$(MAKE) -sf build-aux/bootstrap.mk --no-print-directory $@

vars:
	@cmake -H. -B$(DIR) -LA

tree:
	@tree build -I "*.cmake|CMake*" --matchdirs -F -a $(if $(full),-f)

build/cache.mk:

$(REBOOTSTR_FILE):
	@echo "Rerun 'make bootstrap'!" && false

.DEFAULT:
	@if [ ! -f build/cache.mk ]; then \
	    $(MAKE) -f build-aux/bootstrap.mk --no-print-directory; \
    else \
        $(generator) -C$(DIR) $(VERBOSE)\
            $(if $(findstring $(generator),ninja),, --no-print-directory)\
            $(if $(jobs),-j$(jobs),-j$(shell nproc)) $@;\
	fi

.PHONY: bootstrap rebootstrap distclean info test doc
