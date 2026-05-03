#include "speculative_parallel.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>
#include <omp.h>

SpeculativeParallelStats speculative_parallel_coloring(const GraphCSR &g) {
  const std::int32_t n = g.n;
  SpeculativeParallelStats out;
  out.color.assign(static_cast<std::size_t>(n), -1);  // -1 means uncolored
  out.rounds = 0;

  std::vector<bool> conflicted(static_cast<std::size_t>(n), false);

  bool done = false;
  while (!done) {
    ++out.rounds;

    // Parallel speculative coloring
    #pragma omp parallel for
    for (std::int32_t u = 0; u < n; ++u) {
      if (out.color[static_cast<std::size_t>(u)] != -1 && !conflicted[static_cast<std::size_t>(u)]) {
        continue;  // already colored and not conflicted
      }
      // Compute forbidden colors from neighbors
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
    }

    // Conflict detection
    std::fill(conflicted.begin(), conflicted.end(), false);
    #pragma omp parallel for
    for (std::int32_t u = 0; u < n; ++u) {
      const std::int64_t beg = g.row_ptr[static_cast<std::size_t>(u)];
      const std::int64_t end = g.row_ptr[static_cast<std::size_t>(u) + 1];
      for (std::int64_t p = beg; p < end; ++p) {
        const std::int32_t v = g.col_idx[static_cast<std::size_t>(p)];
        if (u < v) {  // to avoid double-checking
          const int cu = out.color[static_cast<std::size_t>(u)];
          const int cv = out.color[static_cast<std::size_t>(v)];
          if (cu == cv && cu != -1) {
            // Mark the one with higher ID as conflicted
            conflicted[static_cast<std::size_t>(std::max(u, v))] = true;
          }
        }
      }
    }

    // Check if any conflicted
    done = true;
    for (bool c : conflicted) {
      if (c) {
        done = false;
        break;
      }
    }
  }

  // Compute num_colors
  int max_color = -1;
  for (int c : out.color) {
    max_color = std::max(max_color, c);
  }
  out.num_colors = max_color + 1;

  return out;
}