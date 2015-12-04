VERBOSE := $(if $(findstring $(verbose),true 1),$(if $(findstring $(generator),ninja),-v,VERBOSE=1))

-include build/cache.mk

all install uninstall test help doc:
	@if [ -d build -a -f build/cache.mk ]; then \
        CTEST_OUTPUT_ON_FAILURE=TRUE $(generator) -C$(DIR) $(VERBOSE) -j$(shell nproc) $@; \
    else \
        $(MAKE) -f bootstrap.mk options; \
    fi

distclean:
	[ -n "$(DIR)" -a -d "$(DIR)" ] && rm -vfr $(DIR) || true

bootstrap:
	$(MAKE) -sf bootstrap.mk $@ $(MAKEFLAGS)

info:
	@$(MAKE) -s -f bootstrap.mk $@

%:
	@[ -n "$(generator)" ] && $(generator) -C $(DIR) -j$(shell nproc) $(VERBOSE) $@ || \
        echo "Not bootstrapped!" && false
    

.PHONY: bootstrap distclean clean all install uninstall test help doc
