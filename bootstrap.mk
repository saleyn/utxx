PROJECT     := $(shell sed -n '/^project/{s/^project. *\([a-zA-Z0-9]\+\).*/\1/p; q}' CMakeLists.txt)
VERSION     := $(shell sed -n '/^project/{s/^.\+VERSION \+//; s/[^\.0-9]\+//; p; q}' CMakeLists.txt)

HOSTNAME    := $(shell hostname)

# Options file is either: .cmake-args.$(HOSTNAME) or .cmake-args
OPT_FILE    := .cmake-args.$(HOSTNAME)
ifeq "$(wildcard $(OPT_FILE))" ""
    OPT_FILE:= .cmake-args
    ifeq "$(wildcard $(OPT_FILE))" ""
        OPT_FILE:="/dev/null"
    endif
endif

BUILD       ?= Debug
build       := $(shell echo $(BUILD) | tr '[:upper:]' '[:lower:]')
toolchain   ?= gcc

# Function that replaces variables in a given entry in a file 
# E.g.: $(call substitute,ENTRY,FILENAME)
substitute   = $(shell sed -n '/^$(1)=/{s!$(1)=!!; s!/\+$$!!;   \
                                       s!@PROJECT@!$(PROJECT)!; \
                                       s!@VERSION@!$(VERSION)!; \
                                       s!@BUILD@!$(build)!; \
                                       p; q}' $(2) 2>/dev/null)
BLD_DIR     := $(call substitute,DIR_BUILD,$(OPT_FILE))
ROOT_DIR    := $(dir $(abspath include))
DEF_BLD_DIR := $(ROOT_DIR:%/=%)/build
DIR         := $(if $(BLD_DIR),$(BLD_DIR),$(DEF_BLD_DIR))
GENTOOL     := $(shell cat $(DIR)/.bldtool 2>/dev/null)
generator   ?= make
generator   := $(if $(GENTOOL),$(GENTOOL),$(generator))
VERBOSE     := $(if $(findstring $(verbose),true 1),$(if $(findstring $(generator),ninja),-v,VERBOSE=1))

options:
	@echo
	@echo "Run: make bootstrap [toolchain=gcc|clang|intel] [build=Debug|Release] \\"
	@echo "                    [generator=ninja|make] [verbose=true]"
	@echo
	@echo "To customize variables for cmake, create a local file with VAR=VALUE pairs:"
	@echo "  $(OPT_FILE)"
	@echo

bootstrap: PREFIX  = $(call substitute,DIR_INSTALL,$(OPT_FILE))
bootstrap: prefix  = $(if $(PREFIX),$(PREFIX),/usr/local)
bootstrap: $(DIR)
    ifeq "$(generator)" ""
		@echo -e "\n\e[1;31mBuild tool not specified!\e[0m\n" && false
    else ifeq "$(shell which $(generator) 2>/dev/null)" ""
		@echo -e "\n\e[1;31mBuild tool $(generator) not found!\e[0m\n" && false
    endif
	@echo "Options file.....: $(OPT_FILE)"
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
        -DCMAKE_USER_MAKE_RULES_OVERRIDE=$(ROOT_DIR)/build-aux/CMakeInit.txt \
        -DCMAKE_INSTALL_PREFIX=$(prefix) \
        -DCMAKE_BUILD_TYPE=$(build) \
	    $(patsubst %,-D%,$(shell T='$(MAKEFLAGS)'; echo $${T#*--})) \
        $(patsubst %,-D%,$(shell [ -f "$${OPT_FILE}" ] && egrep -v "^DIR_" $$F | xargs))
	@rm -f build install
	@ln -s $(DIR) build
	@ln -s $(prefix) install
	@echo -e "PROJECT=$(PROJECT)\nVERSION=$(VERSION)" > $(DIR)/cache.mk
	@echo -e "OPT_FILE=$(abspath $(OPT_FILE))"       >> $(DIR)/cache.mk
	@echo -e "generator=$(generator)\nbuild=$(build)\nDIR=$(DIR)\nprefix=$(prefix)" >> $(DIR)/cache.mk

$(DIR):
	@mkdir -p $@

info:
	@echo "PROJECT:  $(PROJECT)"
	@echo "HOSTNAME: $(HOSTNAME)"
	@echo "VERSION:  $(VERSION)"
	@echo "OPT_FILE: $(OPT_FILE)"
	@echo "BLD_DIR:  $(BLD_DIR)"
	@echo "PREFIX:   $(call substitute,DIR_INSTALL,$(OPT_FILE))"
	@echo "GENTOOL:  $(GENTOOL)"
	@echo "maketool: $(generator)"

.PHONY: bootstrap info options
