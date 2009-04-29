include gmsl/gmsl

dollars=$$

SHELL := /bin/bash

# arg 1: names
define include_sub_makes
$$(foreach name,$(1),$$(eval $$(call include_sub_make,$$(name))))
endef

# arg 1: name
# arg 2: dir (optional, is the same as $(1) if not given)
# arg 3: makefile (optional, is $(2)/$(1).mk if not given)
define include_sub_make
$(if $(trace3),$$(warning called include_sub_make "$(1)" "$(2)" "$(3)" CWD=$(CWD)))
DIRNAME:=$(if $(2),$(2),$(1))
MAKEFILE:=$(if $(3),$(3),$(1).mk)
$$(call push,DIRS,$$(call peek,DIRS)$$(if $$(call peek,DIRS),/,)$$(DIRNAME))
CWD:=$$(call peek,DIRS)
include $$(if $$(CWD),$$(CWD)/,)/$$(MAKEFILE)
$$(CWD_NAME)_SRC :=	$(SRC)/$$(CWD)
$$(CWD_NAME)_OBJ :=	$(OBJ)/$$(CWD)
#$$(warning stack contains $(__gmsl_stack_DIRS))
CWD:=$$(call pop,DIRS)
endef

# add a c++ source file
# $(1): filename of source file
# $(2): basename of the filename
define add_c++_source
$(if $(trace),$$(warning called add_c++_source "$(1)" "$(2)"))
BUILD_$(CWD)/$(2).lo_COMMAND:=$(CXX) $(CXXFLAGS) -o $(OBJ)/$(CWD)/$(2).lo -c $(SRC)/$(CWD)/$(1) -MP -MMD -MF $(OBJ)/$(CWD)/$(2).d -MQ $(OBJ)/$(CWD)/$(2).lo $$(OPTIONS_$(CWD)/$(1))
$(if $(trace),$$(warning BUILD_$(CWD)/$(2).lo_COMMAND := "$$(BUILD_$(CWD)/$(2).lo_COMMAND)"))
$(OBJ)/$(CWD)/$(2).d:
$(OBJ)/$(CWD)/$(2).lo:	$(SRC)/$(CWD)/$(1) $(OBJ)/$(CWD)/.dir_exists
	$$(if $(verbose_build),@echo $$(BUILD_$(CWD)/$(2).lo_COMMAND),@echo "[C++] $(CWD)/$(1)")
	@$$(BUILD_$(CWD)/$(2).lo_COMMAND) || (echo "FAILED += $$@" >> .target.mk && false)
	@if [ -f $(2).d ] ; then mv $(2).d $(OBJ)/$(CWD)/$(2).d; fi

-include $(OBJ)/$(CWD)/$(2).d
endef

# add a fortran source file
define add_fortran_source
$(if $(trace),$$(warning called add_fortran_source "$(1)" "$(2)"))
BUILD_$(CWD)/$(2).lo_COMMAND:=$(FC) $(FFLAGS) -o $(OBJ)/$(CWD)/$(2).lo -c $(SRC)/$(CWD)/$(1) -MP -MMD
$(if $(trace),$$(warning BUILD_$(CWD)/$(2).lo_COMMAND := "$$(BUILD_$(CWD)/$(2).lo_COMMAND)"))
$(OBJ)/$(CWD)/$(2).d:
$(OBJ)/$(CWD)/$(2).lo:	$(SRC)/$(CWD)/$(1) $(OBJ)/$(CWD)/.dir_exists
	$$(if $(verbose_build),@echo $$(BUILD_$(CWD)/$(2).lo_COMMAND),@echo "[FORTRAN] $(CWD)/$(1)")
	@$$(BUILD_$(CWD)/$(2).lo_COMMAND) || (echo "FAILED += $$@" >> .target.mk && false)


-include $(OBJ)/$(CWD)/$(2).d
endef

define add_cuda_source
$(if $(trace),$$(warning called add_cuda_source "$(1)" "$(2)"))
$(OBJ)/$(CWD)/$(2).d: $(SRC)/$(CWD)/$(1) $(OBJ)/$(CWD)/.dir_exists
	($(NVCC) $(NVCCFLAGS) -M $$< | awk 'NR == 1 { print "$(OBJ)/$(CWD)/$(2).lo", "$$@", ":", $$$$3, "\\"; next; } /usr/ { next; } { print; }'; echo) > $$@~
	mv $$@~ $$@

BUILD_$(CWD)/$(2).lo_COMMAND:=$(NVCC) $(NVCCFLAGS) -c -o $(OBJ)/$(CWD)/$(2).lo --verbose $(SRC)/$(CWD)/$(1)
$(if $(trace),$$(warning BUILD_$(CWD)/$(2).lo_COMMAND := "$$(BUILD_$(CWD)/$(2).lo_COMMAND)"))

$(OBJ)/$(CWD)/$(2).lo:	$(SRC)/$(CWD)/$(1) $(OBJ)/$(CWD)/.dir_exists
	$$(if $(verbose_build),@echo $$(BUILD_$(CWD)/$(2).lo_COMMAND),@echo "[CUDA] $(CWD)/$(1)")
	@$$(BUILD_$(CWD)/$(2).lo_COMMAND) || (echo "FAILED += $$@" >> .target.mk && false)


-include $(OBJ)/$(CWD)/$(2).d
endef

# Set up the map to map an extension to the name of a function to call
$(call set,EXT_FUNCTIONS,.cc,add_c++_source)
$(call set,EXT_FUNCTIONS,.f,add_fortran_source)
$(call set,EXT_FUNCTIONS,.cu,add_cuda_source)

# add a single source file
# $(1): filename
# $(2): suffix of the filename
define add_source
$$(if $(trace),$$(warning called add_source "$(1)" "$(2)"))
$$(if $$(ADDED_SOURCE_$(CWD)_$(1)),,\
    $$(if $$(call defined,EXT_FUNCTIONS,$(2)),\
	$$(eval $$(call $$(call get,EXT_FUNCTIONS,$(2)),$(1),$$(basename $(1))))\
	    $$(eval ADDED_SOURCE_$(CWD)_$(1):=$(true)),\
	$$(error Extension "$(2)" is not known adding source file $(1))))
endef


# add a list of source files
# $(1): list of filenames
define add_sources
$$(if $(trace),$$(warning called add_sources "$(1)"))
$$(foreach file,$$(strip $(1)),$$(eval $$(call add_source,$$(file),$$(suffix $$(file)))))
endef

# set compile options for a single source file
# $(1): filename
# $(2): compile option
define set_single_compile_option
OPTIONS_$(CWD)/$(1) += $(2)
#$$(warning setting OPTIONS_$(CWD)/$(1) += $(2))
endef

# set compile options for a given list of source files
# $(1): list of filenames
# $(2): compile option
define set_compile_option
$$(foreach file,$(1),$$(eval $$(call set_single_compile_option,$$(file),$(2))))
endef

# add a library
# $(1): name of the library
# $(2): source files to include in the library
# $(3): libraries to link with

define library
$$(if $(trace),$$(warning called library "$(1)" "$(2)" "$(3)"))
$$(eval $$(call add_sources,$(2)))

OBJFILES_$(1):=$(addsuffix .lo,$(basename $(2:%=$(OBJ)/$(CWD)/%)))
LINK_$(1)_COMMAND:=$(CXX) $(CXXFLAGS) $(CXXLINKFLAGS) -o $(BIN)/lib$(1).so $$(OBJFILES_$(1)) $$(foreach lib,$(3), -l$$(lib))

$(BIN)/lib$(1).so:	$(BIN)/.dir_exists $$(OBJFILES_$(1)) $(foreach lib,$(3),$$(LIB_$(lib)_DEPS))
	$$(if $(verbose_build),@echo $$(LINK_$(1)_COMMAND),@echo "[SO] lib$(1).so")
	@$$(LINK_$(1)_COMMAND) || (echo "FAILED += $$@" >> .target.mk && false)

LIB_$(1)_DEPS := $(BIN)/lib$(1).so

libraries: $(BIN)/lib$(1).so

endef


# add a program
# $(1): name of the program
# $(2): libraries to link with
# $(3): name of files to include in the program.  If not included or empty,
#       $(1).cc assumed
# $(4): list of targets to add this program to
define program
$$(if $(trace4),$$(warning called program "$(1)" "$(2)" "$(3)"))

$(1)_PROGFILES:=$$(if $(3),$(3),$(1:%=%.cc))
$(1)_OBJFILES:=$$(addsuffix .lo,$$(basename $$($(1)_PROGFILES:%=$(OBJ)/$(CWD)/%)))
#$$(warning $(1)_PROGFILES = "$$($(1)_PROGFILES)")
$$(eval $$(call add_sources,$$($(1)_PROGFILES)))

LINK_$(1)_COMMAND:=$(CXX) $(CXXFLAGS) $(CXXEXEFLAGS) -o $(BIN)/$(1) $(OBJ)/arch/exception_hook.lo -ldl $$(foreach lib,$(2), -l$$(lib)) $$($(1)_OBJFILES)

$(BIN)/$(1):	$(BIN)/.dir_exists $$($(1)_OBJFILES) $(foreach lib,$(2),$$(LIB_$(lib)_DEPS)) $(OBJ)/arch/exception_hook.lo
	$$(if $(verbose_build),@echo $$(LINK_$(1)_COMMAND),@echo "[BIN] $(1)")
	@$$(LINK_$(1)_COMMAND)

$$(foreach target,$(4) programs,$$(eval $$(target): $(BIN)/$(1)))

$(1): $(BIN)/$(1)
.PHONY:	$(1)

endef

# add a test case
# $(1) name of the test
# $(2) libraries to link with
# $(3) test style.  boost = boost test framework

define test
$$(if $(trace),$$(warning called test "$(1)" "$(2)" "$(3)"))

$$(eval $$(call add_sources,$(1).cc))

$(1)_OBJFILES:=$(OBJ)/$(CWD)/$(1).lo

LINK_$(1)_COMMAND:=$(CXX) $(CXXFLAGS) $(CXXEXEFLAGS) -o $(TESTS)/$(1) $(OBJ)/arch/exception_hook.lo -ldl $$(foreach lib,$(2), -l$$(lib)) $(OBJ)/$(CWD)/$(1).lo $(if $(findstring $(3),boost), -lboost_unit_test_framework-mt)

$(TESTS)/$(1):	$(TESTS)/.dir_exists $(OBJ)/$(CWD)/$(1).lo $(foreach lib,$(2),$$(LIB_$(lib)_DEPS)) $(OBJ)/arch/exception_hook.lo
	$$(if $(verbose_build),@echo $$(LINK_$(1)_COMMAND),@echo "[BIN] $(1)")
	@$$(LINK_$(1)_COMMAND)

tests:	$(TESTS)/$(1)

TEST_$(1)_COMMAND := rm -f $(TESTS)/$(1).{passed,failed} && ((set -o pipefail && $(TESTS)/$(1) > $(TESTS)/$(1).running 2>&1 && mv $(TESTS)/$(1).running $(TESTS)/$(1).passed) || (mv $(TESTS)/$(1).running $(TESTS)/$(1).failed && echo "           $(1) FAILED" && cat $(TESTS)/$(1).failed && false))

$(TESTS)/$(1).passed:	$(TESTS)/$(1)
	$$(if $(verbose_build),@echo '$$(TEST_$(1)_COMMAND)',@echo "[TESTCASE] $(1)")
	@$$(TEST_$(1)_COMMAND)

$(1):	$(TESTS)/$(1)
	$(TESTS)/$(1)

test:	$(TESTS)/$(1).passed

endef