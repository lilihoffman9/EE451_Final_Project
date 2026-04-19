#include "greedy_sequential.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

GreedySequentialStats greedy_sequential_first_fit(const GraphCSR &g) {
  const std::int32_t n = g.n;
  GreedySequentialStats out;
  out.color.assign(static_cast<std::size_t>(n), 0);

  // One bit per possible color index in [0, n); greedy uses at most n colors.
  const std::size_t num_words = (static_cast<std::size_t>(n) + 63u) / 64u;
  std::vector<std::uint64_t> forbidden(num_words, 0);
  std::vector<int> touched;
  touched.reserve(64);

  int max_color = -1;

  for (std::int32_t u = 0; u < n; ++u) {
    touched.clear();
    const std::int64_t beg = g.row_ptr[static_cast<std::size_t>(u)];
    const std::int64_t end = g.row_ptr[static_cast<std::size_t>(u) + 1];
    for (std::int64_t p = beg; p < end; ++p) {
      const std::int32_t v = g.col_idx[static_cast<std::size_t>(p)];
      if (v >= u) {
        continue;
      }
      const int c = out.color[static_cast<std::size_t>(v)];
      const std::size_t wi = static_cast<std::size_t>(c) >> 6;
      const unsigned shift = static_cast<unsigned>(c) & 63u;
      std::uint64_t &w = forbidden[wi];
      if (((w >> shift) & 1ULL) == 0) {
        w |= 1ULL << shift;
        touched.push_back(c);
      }
    }

    int c = 0;
    for (;; ++c) {
      const std::size_t wi = static_cast<std::size_t>(c) >> 6;
      const unsigned shift = static_cast<unsigned>(c) & 63u;
      if (((forbidden[wi] >> shift) & 1ULL) == 0) {
        break;
      }
    }
    out.color[static_cast<std::size_t>(u)] = c;
    max_color = std::max(max_color, c);

    for (const int t : touched) {
      const std::size_t wi = static_cast<std::size_t>(t) >> 6;
      const unsigned shift = static_cast<unsigned>(t) & 63u;
      forbidden[wi] &= ~(1ULL << shift);
    }
  }

  out.num_colors = max_color + 1;
  return out;
}
