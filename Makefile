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

.PHONY: bootstrap rebootstrap distclean info test doc \
        reactor reactor-tests reactor-clean

#-------------------------------------------------------------------------------
# Standalone reactor build (no CMake / boost required)
#-------------------------------------------------------------------------------
REACTOR_INC    := include
REACTOR_CXX    := g++
REACTOR_CXXFLAGS := -std=c++17 -I$(REACTOR_INC) -Wall -Wextra -Wpedantic \
                    $(if $(findstring $(build),debug),-g -O0,-O2 -DNDEBUG)
REACTOR_LIBS   := -lrt -laio
REACTOR_OUTDIR := .build/reactor

REACTOR_SRCS   := src/reactor/reactor_platform.cpp \
                  src/reactor/reactor_misc.cpp \
                  src/reactor/reactor_fd_info.cpp \
                  src/reactor/reactor.cpp

REACTOR_TEST_SRCS := test/reactor/test_compat.cpp \
                     test/reactor/test_reactor_types.cpp \
                     test/reactor/test_reactor_misc.cpp \
                     test/reactor/test_aio_reader.cpp \
                     test/reactor/test_reactor.cpp

REACTOR_OBJS  := $(patsubst src/%.cpp,$(REACTOR_OUTDIR)/%.o,$(REACTOR_SRCS))
REACTOR_BINS  := $(patsubst test/reactor/%.cpp,$(REACTOR_OUTDIR)/%,$(REACTOR_TEST_SRCS))

$(REACTOR_OUTDIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(REACTOR_CXX) $(REACTOR_CXXFLAGS) -c $< -o $@

$(REACTOR_OUTDIR)/test_%: test/reactor/test_%.cpp $(REACTOR_OBJS)
	@mkdir -p $(dir $@)
	$(REACTOR_CXX) $(REACTOR_CXXFLAGS) $^ $(REACTOR_LIBS) -o $@

## Build all reactor object files and test binaries
reactor: $(REACTOR_BINS)

## Run all reactor test binaries; print per-suite pass/fail counts
reactor-tests: $(REACTOR_BINS)
	@pass=0; fail=0; \
	for t in $(REACTOR_BINS); do \
		echo "=== $$t ==="; \
		out=$$($$t 2>&1); rc=$$?; echo "$$out"; \
		p=$$(echo "$$out" | grep -oP '\d+(?= passed)' | tail -1); \
		f=$$(echo "$$out" | grep -oP '\d+(?= failed)' | tail -1); \
		pass=$$((pass + $${p:-0})); \
		fail=$$((fail + $${f:-0})); \
		[ $$rc -eq 0 ] || fail=$$((fail + 1)); \
	done; \
	echo; \
	echo "Total: $$pass passed, $$fail failed"; \
	[ $$fail -eq 0 ]

## Remove reactor build artifacts
reactor-clean:
	rm -rf $(REACTOR_OUTDIR)
