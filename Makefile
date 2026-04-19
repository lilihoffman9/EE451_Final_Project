# EE451 graph coloring — build from repo root
# Layout: algorithms/ = coloring implementations; supportingFiles/ = I/O, graph, driver, scripts
# Usage:
#   make
#   make run GRAPH=data/email-EuAll.txt THREADS=8
#   ./supportingFiles/scripts/bench_sequential_all.sh
#
# Datasets: download_snap_datasets.sh skips files that already exist (re-run is cheap).
# On cloud, set EE451_DATA_DIR to persistent storage so graphs survive instance refresh, e.g.:
#   export EE451_DATA_DIR=/mnt/your-ebs/snap
#
# macOS (Homebrew): brew install libomp

CXX ?= g++
CPPFLAGS += -IsupportingFiles/include -Ialgorithms
CXXFLAGS += -std=c++17 -O3 -Wall -Wextra

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
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
GRAPH ?= supportingFiles/fixtures/tiny_edges.txt

OBJS := build/main.o build/load_snap.o build/validate.o build/greedy_sequential.o

.PHONY: all clean rebuild run

all: bin/coloring

rebuild: clean all

bin/coloring: $(OBJS) | bin
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

bin:
	mkdir -p bin

build:
	mkdir -p build

build/main.o: supportingFiles/src/main.cpp | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

build/load_snap.o: supportingFiles/src/load_snap.cpp | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

build/validate.o: supportingFiles/src/validate.cpp | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

build/greedy_sequential.o: algorithms/greedy_sequential.cpp | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -rf build bin

run: all
	OMP_NUM_THREADS=$(THREADS) ./bin/coloring $(GRAPH) $(THREADS)

-include build/main.d build/load_snap.d build/validate.d build/greedy_sequential.d
