ifeq "$(CPPUTEST_HOME)" ""
$(error The environment variable CPPUTEST_HOME is not set. \
Set it to where cpputest is installed)
endif

# we don’t want to use relative paths, so we set these variables
CODE_DIR = 	$(HOME)/newCode
LIB_DIR = 	$(CODE_DIR)/commonLibs
PROJECT_DIR =	$(LIB_DIR)/BitsOfData
SRC_DIR =	$(PROJECT_DIR)/src
TEST_DIR =	$(PROJECT_DIR)/test
MOCK_DIR = 	$(PROJECT_DIR)/mocks

# specify where the source code and includes are located
INCLUDE_DIRS += $(SRC_DIR)
INCLUDE_DIRS += $(CPPUTEST_HOME)/include
INCLUDE_DIRS += $(CPPUTEST_HOME)/include/Platforms/Gcc
INCLUDE_DIRS += $(MOCK_DIR)
#INCLUDE_DIRS += $(LIB_DIR)/Utilities/src
#INCLUDE_DIRS += $(LIB_DIR)/Hardware/arch/X86

SRC_DIRS +=	$(SRC_DIR)
TEST_SRC_DIRS =	$(TEST_DIR)
TEST_SRC_DIRS += $(MOCK_DIR)
#TEST_SRC_DIRS += $(LIB_DIR)/Hardware/arch/X86

# what to call the test binary
TEST_TARGET = testAll

# Some flags to silence clang
ifeq ($(shell $(CC) -v 2>&1 | grep -c "clang"), 1)
CPPUTEST_WARNINGFLAGS += -Wno-unknown-warning-option
CPPUTEST_WARNINGFLAGS += -Wno-covered-switch-default
CPPUTEST_WARNINGFLAGS += -Wno-reserved-id-macro
CPPUTEST_WARNINGFLAGS += -Wno-keyword-macro
CPPUTEST_WARNINGFLAGS += -Wno-documentation
CPPUTEST_WARNINGFLAGS += -Wno-missing-noreturn
CPPUTEST_WARNINGFLAGS += -Wno-poison-system-directories
CPPUTEST_WARNINGFLAGS += -Wno-inconsistent-missing-destructor-override
CPPUTEST_WARNINGFLAGS += -Wno-suggest-destructor-override
CPPUTEST_WARNINGFLAGS += -Wno-suggest-override
CPPUTEST_WARNINGFLAGS += -Wno-zero-as-null-pointer-constant
endif

CPPUTEST_WARNINGFLAGS += -Wall
CPPUTEST_WARNINGFLAGS += -Werror
CPPUTEST_WARNINGFLAGS += -Wfatal-errors
CPPUTEST_WARNINGFLAGS += -Wswitch-default
CPPUTEST_WARNINGFLAGS += -Wno-format-nonliteral
CPPUTEST_WARNINGFLAGS += -Wno-declaration-after-statement
CPPUTEST_WARNINGFLAGS += -Wno-sign-conversion
CPPUTEST_WARNINGFLAGS += -Wno-pedantic
CPPUTEST_WARNINGFLAGS += -Wno-shadow
CPPUTEST_WARNINGFLAGS += -Wno-missing-field-initializers
CPPUTEST_WARNINGFLAGS += -Wno-unused-parameter
CPPUTEST_WARNINGFLAGS += -Wno-inconsistent-missing-destructor-override
CPPUTEST_WARNINGFLAGS += -Wno-suggest-destructor-override
CPPUTEST_WARNINGFLAGS += -Wno-suggest-override
CPPUTEST_WARNINGFLAGS += -Wno-switch-enum
CPPUTEST_WARNINGFLAGS += -Wno-zero-as-null-pointer-constant
CPPUTEST_CFLAGS += -pedantic
CPPUTEST_CFLAGS += -Wno-missing-prototypes
CPPUTEST_CFLAGS += -Wno-strict-prototypes
CPPUTEST_CXXFLAGS += -Wno-c++14-compat
CPPUTEST_CXXFLAGS += --std=c++11
CPPUTEST_CXXFLAGS += -Wno-c++98-compat-pedantic
CPPUTEST_CXXFLAGS += -Wno-c++98-compat

# run MakefileWorker.mk with the variables defined here
include $(CPPUTEST_HOME)/build/MakefileWorker.mk

CXXFLAGS += -include $(CPPUTEST_HOME)/include/CppUTest/MemoryLeakDetectorNewMacros.h
CFLAGS += -include $(CPPUTEST_HOME)/include/CppUTest/MemoryLeakDetectorMallocMacros.h
CFLAGS += -g
CFLAGS += -fms-extensions
