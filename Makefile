PROJECT     := utxx

# Function that replaces variables in a given entry in a file 
# E.g.: $(call substitute,ENTRY,FILENAME)
substitute   = $(shell sed -n '/^$(1)=/{s!$(1)=!!; s!/\+$$!!; \
                                       s!@PROJECT@!$(PROJECT)!; \
                                       s!@VERSION@!$(VERSION)!; p; q}' $(2) 2>/dev/null)
HOSTNAME    := $(shell hostname)
OPT_FILE    := .cmake-args.$(HOSTNAME)
VERSION     := $(shell sed -n '/^project/{s/^.\+VERSION \+//; s/[^\.0-9]\+//; p; q}' CMakeLists.txt)
BLD_DIR     := $(call substitute,DIR_BUILD,$(OPT_FILE))
DEF_BLD_DIR := $(dir $(abspath include))
DEF_BLD_DIR := $(DEF_BLD_DIR:%/=%)/build
DIR         := $(if $(BLD_DIR),$(BLD_DIR),$(DEF_BLD_DIR))
build       ?= Debug
generator   ?= make
GENTOOL     := $(shell cat $(DIR)/.bldtool 2>/dev/null)
generator   := $(if $(GENTOOL),$(GENTOOL),$(generator))
VERBOSE     := $(if $(findstring $(verbose),true 1),$(if $(findstring $(generator),ninja),-v,VERBOSE=1))
toolchain   ?= gcc

all install uninstall test help doc:
	@if [ -f $(DIR)/.bldtool ]; then \
        CTEST_OUTPUT_ON_FAILURE=TRUE $(generator) -C$(DIR) $(VERBOSE) -j$(shell nproc) $@; \
    else \
    	echo "Run: make bootstrap [toolchain=gcc|clang|intel] [build=Debug|Release] \\"; \
        echo "                    [generator=ninja|make] [verbose=true]"; \
    	echo; \
    	echo "To customize variables for cmake, create a local file with VAR=VALUE pairs:"; \
    	echo "  $(OPT_FILE)"; \
    fi

bootstrap: PREFIX  = $(call substitute,DIR_INSTALL,$(OPT_FILE))
bootstrap: prefix  = $(if $(PREFIX),$(PREFIX),/usr/local)
bootstrap: $(DIR)
    ifeq "$(generator)" ""
		@echo -e "\n\e[1;31mBuild tool not specified!\e[0m\n" && false
    else ifeq "$(shell which $(generator) 2>/dev/null)" ""
		@echo -e "\n\e[1;31mBuild tool $(generator) not found!\e[0m\n" && false
    endif
	@echo "Build directory..: $(DIR)"
	@echo "Install directory: $(prefix)"
	@echo "Build type.......: $(build)"
	@echo "Generator........: $(generator)"
	@echo $(generator) > $(DIR)/.bldtool
	@echo -e "\n-- \e[1;37mUsing $$(cat $(DIR)/.bldtool) generator\e[0m\n"
	cmake -H. -B$(DIR) $(if $(verbose),-DCMAKE_VERBOSE_MAKEFILE=true) \
        $(if $(findstring $(generator),ninja),-GNinja,-G"Unix Makefiles") \
        --no-warn-unused-cli \
        -DTOOLCHAIN=$(toolchain) \
        -DCMAKE_USER_MAKE_RULES_OVERRIDE=$(DIR)/CMakeInit.txt \
        -DCMAKE_INSTALL_PREFIX=$(prefix) \
        -DCMAKE_BUILD_TYPE=$(build) \
        $(patsubst %,-D%,$(filter-out --,$(MAKEFLAGS))) \
        $(patsubst %,-D%,$(shell F=.cmake-args.$$HOSTNAME && [ -f "$$F" ] && egrep -v "^DIR_" $$F | xargs))

distclean:
	rm -fr $(DIR)

$(DIR):
	@mkdir -p $@
	@cp build-aux/CMake*.txt $@

%:
	$(generator) -C $(DIR) -j$(shell nproc) $(VERBOSE) $@

info:
	@echo "HOSTNAME: $(HOSTNAME)"
	@echo "VERSION:  $(VERSION)"
	@echo "OPT_FILE: $(OPT_FILE)"
	@echo "BLD_DIR:  $(BLD_DIR)"
	@echo "PREFIX:   $(call substitute,DIR_INSTALL,$(OPT_FILE))"
	@echo "maketool: $(maketool)"
	@echo "bldtool:  $(bldtool)"

.PHONY: bootstrap distclean clean all install uninstall test help doc
