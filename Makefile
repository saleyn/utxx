-include build/cache.mk

VERBOSE := $(if $(findstring $(verbose),true 1),$(if $(findstring $(generator),ninja),-v,VERBOSE=1))

all install uninstall help doc edit_cache:
	@if [ -d build -a -f build/cache.mk ]; then \
	    $(generator) -C$(DIR) $(VERBOSE) -j$(shell nproc) $@; \
    else \
        $(MAKE) -sf bootstrap.mk; \
    fi

distclean:
	[ -n "$(DIR)" -a -d "$(DIR)" ] && rm -vfr $(DIR) || true

bootstrap:
	$(MAKE) -sf bootstrap.mk $@ $(MAKEFLAGS)

info:
	@$(MAKE) -sf bootstrap.mk $@

test:
	CTEST_OUTPUT_ON_FAILURE=TRUE $(generator) -C$(DIR) $(VERBOSE) -j$(shell nproc) $@

.PHONY: bootstrap distclean clean all install uninstall test help doc
