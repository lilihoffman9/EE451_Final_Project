#include "graph_csr.hpp"

#include <stdexcept>
#include <vector>

bool validate_coloring(const GraphCSR &g, const std::vector<int> &color) {
  if (static_cast<std::int32_t>(color.size()) != g.n) {
    throw std::invalid_argument("color.size() must equal g.n");
  }
  for (std::int32_t u = 0; u < g.n; ++u) {
    const std::int64_t beg = g.row_ptr[static_cast<std::size_t>(u)];
    const std::int64_t end = g.row_ptr[static_cast<std::size_t>(u) + 1];
    for (std::int64_t p = beg; p < end; ++p) {
      const std::int32_t v = g.col_idx[static_cast<std::size_t>(p)];
      if (color[static_cast<std::size_t>(u)] == color[static_cast<std::size_t>(v)]) {
        return false;
      }
    }
  }
  return true;
}
