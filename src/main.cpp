#include "graph_csr.hpp"
#include "timing.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <snap_edge_list.txt> [threads]\n";
    return 1;
  }
  const std::string path = argv[1];
  if (argc >= 3) {
    if (setenv("OMP_NUM_THREADS", argv[2], 1) != 0) {
      std::cerr << "Warning: could not set OMP_NUM_THREADS\n";
    }
  }

  GraphCSR g = load_snap_edge_list(path);
  std::cout << "Loaded " << path << "\n";
  std::cout << "  n (vertices): " << g.n << "\n";
  std::cout << "  m (undirected edges): " << g.num_undirected_edges << "\n";
  std::cout << "  nnz (CSR entries): " << g.row_ptr[static_cast<std::size_t>(g.n)] << "\n";

  // Placeholder: trivial invalid coloring (all 0) to exercise the validator on non-edgeless graphs.
  std::vector<int> color(static_cast<std::size_t>(g.n), 0);
  const bool ok = validate_coloring(g, color);
  std::cout << "Trivial all-zero coloring valid? " << (ok ? "yes" : "no") << " (expected no if m > 0)\n";

  // Timing harness smoke test: measure a cheap parallel loop (replace with coloring later).
  const double t_med =
      median_wall_seconds([&]() {
#pragma omp parallel for schedule(static)
        for (std::int32_t i = 0; i < g.n; ++i) {
          (void)g.row_ptr[static_cast<std::size_t>(i)];
        }
      },
                          5);
  std::cout << "Median wall time (5 runs, placeholder parallel loop): " << t_med << " s\n";

  return 0;
}
