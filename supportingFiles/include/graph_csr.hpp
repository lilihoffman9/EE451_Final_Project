#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
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

// Returns std::nullopt iff coloring is proper: for every edge (u,v), color[u] != color[v].
// Otherwise returns one directed half-edge (u,v) from CSR where colors match (including u==v if self-loop).
std::optional<std::pair<std::int32_t, std::int32_t>> first_monochromatic_edge(const GraphCSR &g,
                                                                             const std::vector<int> &color);

bool validate_coloring(const GraphCSR &g, const std::vector<int> &color);
