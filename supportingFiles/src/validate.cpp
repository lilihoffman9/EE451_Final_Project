#include "graph_csr.hpp"

#include <stdexcept>
#include <vector>

std::optional<std::pair<std::int32_t, std::int32_t>> first_monochromatic_edge(const GraphCSR &g,
                                                                              const std::vector<int> &color) {
  if (static_cast<std::size_t>(g.n) != color.size()) {
    throw std::invalid_argument("color.size() must equal g.n");
  }
  if (g.n == 0) {
    return std::nullopt;
  }
  // Scan every stored half-edge (u,v). Undirected CSR lists each edge twice; any conflict is detected.
  for (std::int32_t u = 0; u < g.n; ++u) {
    const std::int64_t beg = g.row_ptr[static_cast<std::size_t>(u)];
    const std::int64_t end = g.row_ptr[static_cast<std::size_t>(u) + 1];
    for (std::int64_t p = beg; p < end; ++p) {
      const std::int32_t v = g.col_idx[static_cast<std::size_t>(p)];
      if (v < 0 || v >= g.n) {
        throw std::runtime_error("first_monochromatic_edge: CSR col_idx out of range [0, n)");
      }
      const int cu = color[static_cast<std::size_t>(u)];
      const int cv = color[static_cast<std::size_t>(v)];
      if (cu == cv) {
        return std::pair<std::int32_t, std::int32_t>{u, v};
      }
    }
  }
  return std::nullopt;
}

bool validate_coloring(const GraphCSR &g, const std::vector<int> &color) {
  return !first_monochromatic_edge(g, color).has_value();
}
