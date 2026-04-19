# EE451 graph coloring — Phase 1 toolchain
# Usage:
#   make                    # build bin/coloring
#   make run GRAPH=data/email-EuAll.txt THREADS=8
#
# macOS (Homebrew): brew install libomp
#   export CXX=clang++
#   export CPPFLAGS="-I$(brew --prefix libomp)/include"
#   export LDFLAGS="-L$(brew --prefix libomp)/lib -lomp"

CXX ?= g++
CPPFLAGS += -Iinclude
CXXFLAGS += -std=c++17 -O3 -Wall -Wextra

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  # Apple Clang: OpenMP comes from Homebrew libomp (`brew install libomp`).
  LIBOMP_PREFIX := $(shell brew --prefix libomp 2>/dev/null)
  ifeq ($(LIBOMP_PREFIX),)
    ifneq ($(wildcard /opt/homebrew/opt/libomp/include/omp.h),)
      LIBOMP_PREFIX := /opt/homebrew/opt/libomp
    else ifneq ($(wildcard /usr/local/opt/libomp/include/omp.h),)
      LIBOMP_PREFIX := /usr/local/opt/libomp
    endif
  endif
  ifneq ($(LIBOMP_PREFIX),)
    CPPFLAGS += -I$(LIBOMP_PREFIX)/include
    LDFLAGS += -L$(LIBOMP_PREFIX)/lib
  endif
  CXXFLAGS += -Xpreprocessor -fopenmp
  LDFLAGS += -lomp
else
  CXXFLAGS += -fopenmp
  LDFLAGS += -fopenmp
endif

THREADS ?= 4
# After `./scripts/download_snap_datasets.sh`, use e.g. data/email-EuAll.txt
GRAPH ?= fixtures/tiny_edges.txt

SRCS := src/main.cpp src/load_snap.cpp src/validate.cpp src/greedy_sequential.cpp
OBJS := $(SRCS:.cpp=.o)

.PHONY: all clean rebuild run

all: bin/coloring

# Use after upgrading libomp/toolchain, or when you expect a full recompile:
rebuild: clean all

bin/coloring: $(OBJS) | bin
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

bin:
	mkdir -p bin

src/%.o: src/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -f $(OBJS) src/*.d bin/coloring

run: all
	OMP_NUM_THREADS=$(THREADS) ./bin/coloring $(GRAPH) $(THREADS)

-include $(OBJS:.o=.d)
