#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Undirected graph in CSR: symmetric adjacency stored once per endpoint (each undirected edge appears twice).
struct GraphCSR {
  std::int32_t n = 0;  // number of vertices, ids in [0, n)
  std::int64_t num_undirected_edges = 0;  // unique unordered pairs (u,v), u != v
  std::vector<std::int64_t> row_ptr;      // size n + 1
  std::vector<std::int32_t> col_idx;
};

// Load a SNAP-style edge list (lines starting with '#' skipped; two integers per line).
// Treats the graph as undirected; self-loops are ignored.
GraphCSR load_snap_edge_list(const std::string &path);

bool validate_coloring(const GraphCSR &g, const std::vector<int> &color);
