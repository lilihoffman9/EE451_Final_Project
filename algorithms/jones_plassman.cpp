#include "jones_plassman.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <numeric>
#include <vector>
#include <omp.h>

JonesPlassmanStats jones_plassman_coloring(const GraphCSR &g) {
  const std::int32_t n = g.n;
  JonesPlassmanStats out;
  out.color.assign(static_cast<std::size_t>(n), -1);
  out.rounds = 0;

  std::vector<int> priority(static_cast<std::size_t>(n));
  std::srand(42);
  for (std::int32_t u = 0; u < n; ++u)
    priority[static_cast<std::size_t>(u)] = std::rand();

  // forbidden only needs to cover colors 0..max_degree, not 0..n
  int max_degree = 0;
  for (std::int32_t u = 0; u < n; ++u) {
    const int deg = static_cast<int>(g.row_ptr[static_cast<std::size_t>(u) + 1] -
                                     g.row_ptr[static_cast<std::size_t>(u)]);
    if (deg > max_degree) max_degree = deg;
  }
  const std::size_t num_words = (static_cast<std::size_t>(max_degree + 1) + 63u) / 64u;

  const int nthreads = omp_get_max_threads();
  std::vector<std::vector<std::uint64_t>> thread_forbidden(
      static_cast<std::size_t>(nthreads),
      std::vector<std::uint64_t>(num_words, 0u));

  std::vector<std::uint8_t> active(static_cast<std::size_t>(n), 1u);

  // higher_count[u] = number of active neighbors that dominate u.
  // Dominance: v dominates u if priority[v] > priority[u], or equal priority with v > u
  // (tie-break by vertex ID for a strict total order).
  // When higher_count[u] drops to 0, u is a local maximum ready to be colored.
  std::vector<std::int32_t> higher_count(static_cast<std::size_t>(n), 0);
  for (std::int32_t u = 0; u < n; ++u) {
    const std::int64_t beg = g.row_ptr[static_cast<std::size_t>(u)];
    const std::int64_t end = g.row_ptr[static_cast<std::size_t>(u) + 1];
    for (std::int64_t p = beg; p < end; ++p) {
      const std::int32_t v = g.col_idx[static_cast<std::size_t>(p)];
      const int pv = priority[static_cast<std::size_t>(v)];
      const int pu = priority[static_cast<std::size_t>(u)];
      if (pv > pu || (pv == pu && v > u))
        higher_count[static_cast<std::size_t>(u)]++;
    }
  }

  // Initial batch: vertices with no dominating active neighbor
  std::vector<std::int32_t> current_batch, next_batch;
  for (std::int32_t u = 0; u < n; ++u)
    if (higher_count[static_cast<std::size_t>(u)] == 0)
      current_batch.push_back(u);

  // Per-thread staging buffers to avoid locks when collecting the next batch
  std::vector<std::vector<std::int32_t>> thread_next(static_cast<std::size_t>(nthreads));

  while (!current_batch.empty()) {
    ++out.rounds;
    const std::int32_t sz = static_cast<std::int32_t>(current_batch.size());

    // Phase 1: color all local maxima in this batch in parallel
    #pragma omp parallel for schedule(dynamic, 256)
    for (std::int32_t i = 0; i < sz; ++i) {
      const std::int32_t u = current_batch[static_cast<std::size_t>(i)];
      std::vector<std::uint64_t> &forbidden =
          thread_forbidden[static_cast<std::size_t>(omp_get_thread_num())];
      std::fill(forbidden.begin(), forbidden.end(), 0u);
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
      int c = 0;
      for (;; ++c) {
        const std::size_t wi = static_cast<std::size_t>(c) >> 6;
        const unsigned shift = static_cast<unsigned>(c) & 63u;
        if (((forbidden[wi] >> shift) & 1ULL) == 0) break;
      }
      out.color[static_cast<std::size_t>(u)] = c;
      active[static_cast<std::size_t>(u)] = 0u;
    }

    // Phase 2: for each newly colored vertex, decrement higher_count of its active
    // neighbors it dominated; push any that hit 0 into the next batch.
    #pragma omp parallel
    {
      std::vector<std::int32_t> &local_next =
          thread_next[static_cast<std::size_t>(omp_get_thread_num())];
      #pragma omp for schedule(dynamic, 256)
      for (std::int32_t i = 0; i < sz; ++i) {
        const std::int32_t u = current_batch[static_cast<std::size_t>(i)];
        const int pu = priority[static_cast<std::size_t>(u)];
        const std::int64_t beg = g.row_ptr[static_cast<std::size_t>(u)];
        const std::int64_t end = g.row_ptr[static_cast<std::size_t>(u) + 1];
        for (std::int64_t p = beg; p < end; ++p) {
          const std::int32_t v = g.col_idx[static_cast<std::size_t>(p)];
          if (!active[static_cast<std::size_t>(v)]) continue;
          const int pv = priority[static_cast<std::size_t>(v)];
          // u dominated v iff priority[u] > priority[v], or equal with u > v
          if (pu > pv || (pu == pv && u > v)) {
            std::int32_t new_count;
            #pragma omp atomic capture
            new_count = --higher_count[static_cast<std::size_t>(v)];
            if (new_count == 0)
              local_next.push_back(v);
          }
        }
      }
    }

    // Merge per-thread next-batch lists
    next_batch.clear();
    for (std::vector<std::int32_t> &t : thread_next) {
      next_batch.insert(next_batch.end(), t.begin(), t.end());
      t.clear();
    }
    current_batch = std::move(next_batch);
  }

  int max_color = -1;
  for (int c : out.color) max_color = std::max(max_color, c);
  out.num_colors = max_color + 1;
  return out;
}
