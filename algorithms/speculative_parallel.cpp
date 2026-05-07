#include "speculative_parallel.hpp"

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <vector>
#include <omp.h>

SpeculativeParallelStats speculative_parallel_coloring(const GraphCSR &g) {
  const std::int32_t n = g.n;
  SpeculativeParallelStats out;
  out.color.assign(static_cast<std::size_t>(n), -1);
  out.rounds = 0;

  // forbidden only needs to cover colors 0..max_degree, not 0..n
  int max_degree = 0;
  for (std::int32_t u = 0; u < n; ++u) {
    const int deg = static_cast<int>(g.row_ptr[static_cast<std::size_t>(u) + 1] -
                                     g.row_ptr[static_cast<std::size_t>(u)]);
    if (deg > max_degree) max_degree = deg;
  }
  const std::size_t num_words = (static_cast<std::size_t>(max_degree + 1) + 63u) / 64u;

  const int nthreads = omp_get_max_threads();

  // Per-thread forbidden bitvector + dirty-word index list.
  // Instead of std::fill on all num_words each vertex, we only zero out the
  // words we actually touched ("dirty words"). For low-degree vertices this
  // saves clearing hundreds of untouched words.
  std::vector<std::vector<std::uint64_t>> thread_forbidden(
      static_cast<std::size_t>(nthreads),
      std::vector<std::uint64_t>(num_words, 0u));
  std::vector<std::vector<std::uint32_t>> thread_dirty(
      static_cast<std::size_t>(nthreads));

  std::vector<std::uint8_t> conflicted(static_cast<std::size_t>(n), 0u);

  // Round 1: color every vertex. Subsequent rounds: only conflicted vertices.
  std::vector<std::int32_t> to_color(static_cast<std::size_t>(n));
  std::iota(to_color.begin(), to_color.end(), 0);

  while (!to_color.empty()) {
    ++out.rounds;
    const std::int32_t sz = static_cast<std::int32_t>(to_color.size());

    // Speculative coloring: each vertex picks the smallest color not used by
    // any already-colored neighbor (races are intentional; conflicts fixed later).
    #pragma omp parallel for schedule(dynamic, 64)
    for (std::int32_t i = 0; i < sz; ++i) {
      const std::int32_t u = to_color[static_cast<std::size_t>(i)];
      const int tid = omp_get_thread_num();
      std::vector<std::uint64_t> &forbidden = thread_forbidden[static_cast<std::size_t>(tid)];
      std::vector<std::uint32_t> &dirty    = thread_dirty[static_cast<std::size_t>(tid)];
      dirty.clear();

      // Build forbidden set; track which words get written
      const std::int64_t beg = g.row_ptr[static_cast<std::size_t>(u)];
      const std::int64_t end = g.row_ptr[static_cast<std::size_t>(u) + 1];
      for (std::int64_t p = beg; p < end; ++p) {
        const std::int32_t v = g.col_idx[static_cast<std::size_t>(p)];
        const int c = out.color[static_cast<std::size_t>(v)];
        if (c != -1) {
          const std::uint32_t wi    = static_cast<std::uint32_t>(c) >> 6;
          const unsigned      shift = static_cast<unsigned>(c) & 63u;
          if (!forbidden[wi]) dirty.push_back(wi);  // first write to this word
          forbidden[wi] |= 1ULL << shift;
        }
      }

      // Find smallest available color: scan 64 bits at a time with ctzll
      int c = 0;
      for (std::size_t wi = 0; wi < num_words; ++wi) {
        const std::uint64_t avail = ~forbidden[wi];
        if (avail) {
          c = static_cast<int>(wi * 64u) + __builtin_ctzll(avail);
          break;
        }
      }
      out.color[static_cast<std::size_t>(u)] = c;

      // Clear only the words we dirtied
      for (const std::uint32_t wi : dirty) forbidden[wi] = 0u;
    }

    // Conflict detection: mark the higher-ID endpoint of each monochromatic edge.
    // Use a parallel reduction to count conflicts so we avoid the O(n) done-scan.
    std::fill(conflicted.begin(), conflicted.end(), 0u);
    int n_conflicts = 0;
    #pragma omp parallel for schedule(dynamic, 256) reduction(+:n_conflicts)
    for (std::int32_t u = 0; u < n; ++u) {
      const std::int64_t beg = g.row_ptr[static_cast<std::size_t>(u)];
      const std::int64_t end = g.row_ptr[static_cast<std::size_t>(u) + 1];
      for (std::int64_t p = beg; p < end; ++p) {
        const std::int32_t v = g.col_idx[static_cast<std::size_t>(p)];
        if (u < v) {
          const int cu = out.color[static_cast<std::size_t>(u)];
          const int cv = out.color[static_cast<std::size_t>(v)];
          if (cu == cv && cu != -1) {
            conflicted[static_cast<std::size_t>(v)] = 1u;
            ++n_conflicts;
          }
        }
      }
    }

    if (n_conflicts == 0) break;

    // Build the to_color list for the next round from conflicted vertices only
    to_color.clear();
    for (std::int32_t u = 0; u < n; ++u)
      if (conflicted[static_cast<std::size_t>(u)])
        to_color.push_back(u);
  }

  int max_color = -1;
  for (int c : out.color) max_color = std::max(max_color, c);
  out.num_colors = max_color + 1;
  return out;
}
