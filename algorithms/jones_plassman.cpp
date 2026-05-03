#include "jones_plassman.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <omp.h>

JonesPlassmanStats jones_plassman_coloring(const GraphCSR &g) {
  const std::int32_t n = g.n;
  JonesPlassmanStats out;
  out.color.assign(static_cast<std::size_t>(n), -1);  // -1 means uncolored
  out.rounds = 0;

  // Assign random priorities
  std::vector<int> priority(static_cast<std::size_t>(n));
  std::srand(42);  // fixed seed for reproducibility
  for (std::int32_t u = 0; u < n; ++u) {
    priority[static_cast<std::size_t>(u)] = std::rand();
  }

  std::vector<bool> active(static_cast<std::size_t>(n), true);  // uncolored vertices

  while (true) {
    ++out.rounds;

    std::vector<bool> can_color(static_cast<std::size_t>(n), false);

    // Parallel: check for each active vertex if it has max priority among active neighbors
    #pragma omp parallel for
    for (std::int32_t u = 0; u < n; ++u) {
      if (!active[static_cast<std::size_t>(u)]) continue;

      bool is_max = true;
      const std::int64_t beg = g.row_ptr[static_cast<std::size_t>(u)];
      const std::int64_t end = g.row_ptr[static_cast<std::size_t>(u) + 1];
      for (std::int64_t p = beg; p < end; ++p) {
        const std::int32_t v = g.col_idx[static_cast<std::size_t>(p)];
        if (active[static_cast<std::size_t>(v)] && priority[static_cast<std::size_t>(v)] > priority[static_cast<std::size_t>(u)]) {
          is_max = false;
          break;
        }
      }
      if (is_max) {
        can_color[static_cast<std::size_t>(u)] = true;
      }
    }

    // Now, color the can_color vertices in parallel
    #pragma omp parallel for
    for (std::int32_t u = 0; u < n; ++u) {
      if (!can_color[static_cast<std::size_t>(u)]) continue;

      // Compute forbidden colors from already colored neighbors
      const std::size_t num_words = (static_cast<std::size_t>(n) + 63u) / 64u;
      std::vector<std::uint64_t> forbidden(num_words, 0);
      const std::int64_t beg = g.row_ptr[static_cast<std::size_t>(u)];
      const std::int64_t end = g.row_ptr[static_cast<std::size_t>(u) + 1];
      for (std::int64_t p = beg; p < end; ++p) {
        const std::int32_t v = g.col_idx[static_cast<std::size_t>(p)];
        const int c = out.color[static_cast<std::size_t>(v)];
        if (c != -1) {
          const std::size_t wi = static_cast<std::size_t>(c) >> 6;
          const unsigned shift = static_cast<unsigned>(c) & 63u;
          forbidden[wi] |= 1ULL << shift;
        }
      }
      // Find smallest available color
      int c = 0;
      for (;; ++c) {
        const std::size_t wi = static_cast<std::size_t>(c) >> 6;
        const unsigned shift = static_cast<unsigned>(c) & 63u;
        if (((forbidden[wi] >> shift) & 1ULL) == 0) {
          break;
        }
      }
      out.color[static_cast<std::size_t>(u)] = c;
      active[static_cast<std::size_t>(u)] = false;
    }

    // Check if all colored
    bool all_colored = true;
    for (bool a : active) {
      if (a) {
        all_colored = false;
        break;
      }
    }
    if (all_colored) break;
  }

  // Compute num_colors
  int max_color = -1;
  for (int c : out.color) {
    max_color = std::max(max_color, c);
  }
  out.num_colors = max_color + 1;

  return out;
}