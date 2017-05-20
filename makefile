
MAKEFLAGS += --warn-undefined-variables

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR := $(BUILD_DIR)/bin
BUILD_DIRS = $(BUILD_DIR) $(OBJ_DIR) $(BIN_DIR)
INC_DIR := include
SRC_TESTS_DIR := tests
SRC_EXAMPLES_DIR := examples

TESTS_SOURCES = $(wildcard $(SRC_TESTS_DIR)/*.cxx)
EXAMPLES_SOURCES = $(wildcard $(SRC_EXAMPLES_DIR)/*.cxx)
TESTS_OBJECTS = $(TESTS_SOURCES:%.cxx=$(OBJ_DIR)/%.o)
EXAMPLES_OBJECTS = $(EXAMPLES_SOURCES:%.cxx=$(OBJ_DIR)/%.o)

$(shell $(CXX) --version | grep -q ^g++ ) 
# FOOB := $(shell $(CXX)  --version ) 

ifeq ($(.SHELLSTATUS),0)
IS_GCC := 1
else
IS_CLANG :=1
endif


CXXFLAGS += -Wall -Wextra -Wshadow -std=c++14 -ggdb3 -fstrict-aliasing -Wstrict-aliasing=1
CXXFLAGS += -fsanitize=undefined -fsanitize=address
CXXFLAGS += -pipe

ifeq ($(IS_GCC), 1)
CXXFLAGS += -Wunsafe-loop-optimizations -Wlogical-op -Wduplicated-cond -Wuninitialized -gdwarf-5
CXXFLAGS += -fvar-tracking -fvar-tracking-assignments
endif


CPPFLAGS += -DBACKWARD_HAS_DW
LDFLAGS += -ldw

DEBUG ?= 1

ifeq ($(DEBUG), 1)
OBJ_DIR := $(addsuffix _debug,$(OBJ_DIR))
else
OBJ_DIR := $(addsuffix _release,$(OBJ_DIR))
CXXFLAGS += -O3
endif

LTO ?= 0

ifeq ($(LTO), 1)
OBJ_DIR := $(addsuffix _lto,$(OBJ_DIR))
CXXFLAGS += -flto=jobserver -flto-odr-type-merging
endif

.SECONDEXPANSION:

#Target specifc variables

.PHONY: clean debug release debugrelease tests

tests: $(BIN_DIR)/tests

.PHONY: $(BIN_DIR)/tests
$(BIN_DIR)/tests: $(TESTS_OBJECTS) | $(BIN_DIR)/
	+$(CXX) $(TESTS_OBJECTS) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ 


$(OBJ_DIR)/%.o: %.cxx | $$(@D)/
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $^ -o $@

# only PRECIOUS can pattern match, not SECONDARY. Old bug.
.PRECIOUS: %/
%/:
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)

# vim:ft=make
#
#
