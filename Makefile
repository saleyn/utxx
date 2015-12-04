PROJECT     := utxx
BLD_DIR     := $(shell sed -n '/^DIR_BUILD=/{s/DIR_BUILD=//p; q}' .cmake-args.$$HOSTNAME 2>/dev/null)
DEF_DIR     := $(dir $(abspath include))
DEF_DIR     := $(DEF_DIR:%/=%)/build
BUILD       ?= Debug
DIR         := $(if $(BLD_DIR),$(BLD_DIR),$(DEF_DIR))
maketool    := $(shell cat $(DIR)/.bldtool 2>/dev/null)
maketool    := $(if $(maketool),$(maketool),make)

toolchain   ?= gcc

ifeq "$(build)" "make"
    bldtool := make
    bldgen  := -G"Unix Makefiles"
else ifeq "$(build)" "ninja"
    bldtool := ninja
    bldgen  := -GNinja
else ifeq "$(build)" ""
    bldtool := ninja
    bldgen  := -GNinja
endif

ifeq "$(verbose)" "true"
    ifeq "$(maketool)" "make"
        VERBOSE="VERBOSE=1"
    else ifeq "$(maketool)" "ninja"
        VERBOSE="-v"
    endif
endif

all install uninstall test help doc:
	@if [ -f $(DIR)/.bldtool ]; then \
        CTEST_OUTPUT_ON_FAILURE=TRUE $(maketool) -C$(DIR) $(VERBOSE) -j$(shell nproc) $@; \
    else \
    	echo "Run: make bootstrap [toolchain=gcc|clang|intel] [build=ninja|make] [verbose=true]"; \
    	echo; \
    	echo "To customize variables for cmake, create a local file with VAR=VALUE pairs:"; \
    	echo "  .cmake-args.\$$HOSTNAME"; \
    fi

bootstrap: VERSION = \
            $(shell sed -n '/^project/{s/^.\+VERSION \+//; s/[^\.0-9]\+//; p; q}' CMakeLists.txt)
bootstrap: PREFIX  = \
            $(shell sed -n '/^DIR_INSTALL=/{s!DIR_INSTALL=!!; s!/\+$$!!; \
                                            s!@PROJECT@!$(PROJECT)!; \
                                            s!@VERSION@!$(VERSION)!; p; q}' \
                    .cmake-args.$$HOSTNAME 2>/dev/null)
bootstrap: prefix = $(if $(PREFIX),$(PREFIX),/usr/local)
bootstrap: $(DIR)
    ifeq "$(bldtool)" ""
		@echo -e "\n\e[1;31mBuild tool $(build) not supported!\e[0m\n" && false
    else ifeq "$(shell which $(bldtool) 2>/dev/null)" ""
		@echo -e "\n\e[1;31mBuild tool $(bldtool) not found!\e[0m\n" && false
    endif
	@echo "Build   directory: $(DIR)"
	@echo "Install directory: $(prefix)"
	@echo $(bldtool) > $(DIR)/.bldtool
	@echo -e "-- \e[1;37mUsing $$(cat $(DIR)/.bldtool) build tool\e[0m\n"
	cmake -H. -B$(DIR) $(if $(verbose),-DCMAKE_VERBOSE_MAKEFILE=true) $(bldgen) \
        --no-warn-unused-cli \
        -DTOOLCHAIN=$(toolchain) \
        -DCMAKE_USER_MAKE_RULES_OVERRIDE=$(DIR)/CMakeInit.txt \
        -DCMAKE_INSTALL_PREFIX=$(prefix) \
        -DCMAKE_BUILD_TYPE=$(BUILD) $(if $(debug),-DDEBUG=vars) \
        $(patsubst %,-D%,$(filter-out --,$(MAKEFLAGS))) \
        $(patsubst %,-D%,$(shell F=.cmake-args.$$HOSTNAME && [ -f "$$F" ] && egrep -v "^DIR_" $$F | xargs))

distclean:
	rm -fr $(DIR)

$(DIR):
	@mkdir -p $@
	@cp build-aux/CMake*.txt $@

%:
	$(maketool) -C $(DIR) -j$(shell nproc) $(if $(verbose),-v) $@

.PHONY: bootstrap distclean clean all install uninstall test help doc
