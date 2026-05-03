#pragma once

#include "graph_csr.hpp"

#include <vector>

struct SpeculativeParallelStats {
  std::vector<int> color;  // color[v] in [0, num_colors)
  int num_colors = 0;      // number of distinct colors used
  int rounds = 0;          // number of recoloring rounds
};

// Speculative parallel coloring: color all vertices in parallel speculatively,
// then detect conflicts and recolor until valid.
SpeculativeParallelStats speculative_parallel_coloring(const GraphCSR &g);