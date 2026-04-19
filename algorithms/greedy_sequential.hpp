#pragma once

#include "graph_csr.hpp"

#include <vector>

struct GreedySequentialStats {
  std::vector<int> color;  // color[v] in [0, num_colors)
  int num_colors = 0;      // number of distinct colors used (= max_color + 1)
};

// First-fit greedy in increasing vertex ID order. For each u, assigns the smallest
// non-negative integer not used by any neighbor v with v < u (already colored).
// Uses a word-backed forbidden-color bitset: bits mark which color indices are blocked.
GreedySequentialStats greedy_sequential_first_fit(const GraphCSR &g);
