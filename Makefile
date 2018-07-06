
MAKEFLAGS       += -swr

# Klee variables
KLEE_BUILDDIR   ?= $$HOME/build/klee/Release+Asserts
KLEE_INCLUDES   ?= $$HOME/build/klee/include/
KLEE_BIN_PATH = $(KLEE_BUILDDIR)/bin
KLEE_LIB_PATH = $(KLEE_BUILDDIR)/lib

# Set some variables to llvm dirs for compilation
LLVM_CONFIG     ?= /usr/bin/llvm-config

TARGET          := libMackeFuzzerOpt.so  # Name of the target
TARGET          := bin/$(TARGET)         # Put the target in bin/ directory

DBGTEST         := $(shell if [ -f .debug ] ; then echo y ; else echo n ; fi)

# General flags/warnings - depends on developer preferences
CXXFLAGS        := -pipe -Wall -Wno-switch -fdiagnostics-color=always

SOURCES         := $(shell find src -name '*.cpp')
HEADERS         := $(shell find src -name '*.h')

HELPER_SOURCES  := helper_funcs/buffer_extract.c

# Specific flags needed for compilation
CXXFLAGS        += $(shell $(LLVM_CONFIG) --cxxflags) -I$(KLEE_INCLUDES) -std=c++14 -funsigned-char
CFLAGS          += $(shell $(LLVM_CONFIG) --cflags) -funsigned-char
LDFLAGS         := -shared $(shell $(LLVM_CONFIG) --ldflags)
LDLIBS          := $(shell $(LLVM_CONFIG) --libs)
DEL             := rm -rfv


ifeq "$(DBGTEST)" ""
CXXFLAGS        += -O3
CFLAGS          += -O3
else
CXXFLAGS        += -g -O0
CFLAGS          += -g -O0
endif



###################################################################################################################################################
# collection of files

OBJS            := $(patsubst src/%.cpp,build/%.o, $(SOURCES))
HELPEROBJS      := $(patsubst %.c,build/%.o, $(HELPER_SOURCES))

###################################################################################################################################################
# end of definitions - start of rules

all: $(TARGET)


ifneq "$(MAKECMDGOALS)" "clean"
ifneq "$(MAKECMDGOALS)" "distclean"
ifneq "$(MAKECMDGOALS)" "edit"
ifneq "$(MAKECMDGOALS)" "test"
DUMMY           := $(shell mkdir -p build)
DUMMY           := $(shell mkdir -p build/helper_funcs)
endif
endif
endif
endif


#Delete implicit rules
%: %.c # delete impicit rule
%: %.cpp # delete impicit rule
%.o: %.c # delete impicit rule
%.o: %.cpp # delete impicit rule
%.o: %.asm # delete impicit rule
%.o: %.S # delete impicit rule


$(TARGET): $(OBJS) $(HELPEROBJS)
	@echo "linking all together ..."
	@mkdir -p $$(dirname $(TARGET))
	$(CXX) $(LDLIBS) $(LDFLAGS) -o $(TARGET) $(OBJS) $(HELPEROBJS) -L$(KLEE_LIB_PATH) -lkleeBasic 


build/helper_funcs/%.o: helper_funcs/%.c
	@echo "compiling $< ..."
	$(CC) $(CFLAGS) -c -o $@ $<

build/%.o: src/%.cpp $(HEADERS)
	@echo "compiling $< ..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<


distclean: clean
	@$(DEL) bin
	@$(DEL) build_fuzz


clean:
	@$(DEL) build

edit:
	$(EDITOR) $(HEADERS) $(SOURCES)


test:
	@echo "-------------------------------------------------------------------------------"
	@echo "SOURCES=$(SOURCES)"
	@echo "-------------------------------------------------------------------------------"
	@echo "HEADERS=$(HEADERS)"
	@echo "-------------------------------------------------------------------------------"
	@echo "OBJS=$(OBJS)"
	@echo "-------------------------------------------------------------------------------"
	@echo "CXXFLAGS=$(CXXFLAGS)"
	@echo "-------------------------------------------------------------------------------"
	@echo "LDFLAGS=$(LDFLAGS)"
	@echo "-------------------------------------------------------------------------------"

