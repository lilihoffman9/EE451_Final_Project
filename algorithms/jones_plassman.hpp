#pragma once

#include "graph_csr.hpp"

#include <vector>

struct JonesPlassmanStats {
  std::vector<int> color;  // color[v] in [0, num_colors)
  int num_colors = 0;      // number of distinct colors used
  int rounds = 0;          // number of rounds
};

// Jones-Plassman parallel coloring: assign random priorities, color in rounds
// where vertices with highest priority among uncolored neighbors are colored.
JonesPlassmanStats jones_plassman_coloring(const GraphCSR &g);