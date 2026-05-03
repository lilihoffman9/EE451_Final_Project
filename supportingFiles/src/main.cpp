#include "graph_csr.hpp"
#include "greedy_sequential.hpp"
#include "speculative_parallel.hpp"
#include "jones_plassman.hpp"
#include "timing.hpp"

#include <omp.h>

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

bool is_positive_int_string(const char *s) {
  if (s == nullptr || *s == '\0') {
    return false;
  }
  for (; *s != '\0'; ++s) {
    if (!std::isdigit(static_cast<unsigned char>(*s))) {
      return false;
    }
  }
  return true;
}

void print_usage(const char *exe) {
  std::cerr << "Usage: " << exe << " <snap_edge_list.txt> [threads] [--brief|-q] [--csv]\n";
  std::cerr << "  Sequential first-fit greedy; T_seq = median of 5 runs after load.\n";
  std::cerr << "  --brief|-q : one result row (aligned columns); --csv : same fields, comma-separated.\n";
}

}  // namespace

int main(int argc, char **argv) {
  std::string path;
  std::string threads_arg;
  bool brief = false;
  bool csv = false;

  for (int i = 1; i < argc; ++i) {
    const char *a = argv[i];
    const std::string as(a);
    if (as == "--brief" || as == "-q") {
      brief = true;
    } else if (as == "--csv") {
      csv = true;
    } else if (as.rfind("--", 0) == 0) {
      std::cerr << "Unknown option: " << as << "\n";
      print_usage(argv[0]);
      return 1;
    } else if (path.empty()) {
      path = as;
    } else if (threads_arg.empty() && is_positive_int_string(a)) {
      threads_arg = as;
    } else {
      print_usage(argv[0]);
      return 1;
    }
  }

  if (path.empty()) {
    print_usage(argv[0]);
    return 1;
  }

  if (!threads_arg.empty()) {
    if (setenv("OMP_NUM_THREADS", threads_arg.c_str(), 1) != 0) {
      std::cerr << "Warning: could not set OMP_NUM_THREADS\n";
    }
  }

  GraphCSR g = load_snap_edge_list(path);
  GreedySequentialStats seq_last;
  const double t_seq =
      median_wall_seconds([&]() { seq_last = greedy_sequential_first_fit(g); }, 5);
  const auto seq_conflict = first_monochromatic_edge(g, seq_last.color);
  const bool seq_ok = !seq_conflict.has_value();

  SpeculativeParallelStats spec_last;
  const double t_spec =
      median_wall_seconds([&]() { spec_last = speculative_parallel_coloring(g); }, 5);
  const auto spec_conflict = first_monochromatic_edge(g, spec_last.color);
  const bool spec_ok = !spec_conflict.has_value();

  JonesPlassmanStats jp_last;
  const double t_jp =
      median_wall_seconds([&]() { jp_last = jones_plassman_coloring(g); }, 5);
  const auto jp_conflict = first_monochromatic_edge(g, jp_last.color);
  const bool jp_ok = !jp_conflict.has_value();

  const std::string dataset = std::filesystem::path(path).filename().string();

  if (brief) {
    if (csv) {
      std::cout << dataset << ',' << g.n << ',' << g.num_undirected_edges << ','
                << (ok ? "ok" : "FAIL") << ',' << last.num_colors << ',';
      std::cout << std::fixed << std::setprecision(9) << t_seq << '\n';
    } else {
      // Fixed-width row so terminals without tab alignment still look like a table.
      const std::string vlabel = ok ? "ok" : "FAIL";
      std::cout << std::left << std::setw(28) << dataset << std::right << std::setw(12) << g.n
                << std::setw(14) << g.num_undirected_edges << std::setw(8) << vlabel << std::setw(8)
                << last.num_colors;
      std::cout << std::setw(16) << std::fixed << std::setprecision(9) << t_seq << '\n';
    }
    return ok ? 0 : 2;
  }

  std::cout << "Loaded " << path << "\n";
  std::cout << "  n (vertices): " << g.n << "\n";
  std::cout << "  m (undirected edges): " << g.num_undirected_edges << "\n";
  std::cout << "  nnz (CSR entries): " << g.row_ptr[static_cast<std::size_t>(g.n)] << "\n";

  std::cout << "Sequential greedy: valid coloring? " << (seq_ok ? "yes" : "no") << "\n";
  if (!seq_ok && seq_conflict.has_value()) {
    const std::int32_t u = seq_conflict->first;
    const std::int32_t v = seq_conflict->second;
    std::cout << "  (invalid: edge/arc in CSR with same color — u=" << u << " v=" << v
              << ", color=" << seq_last.color[static_cast<std::size_t>(u)] << ")\n";
  }
  std::cout << "  colors used: " << seq_last.num_colors << "\n";
  {
    std::cout << std::fixed << std::setprecision(9);
    std::cout << "  T_seq (median of 5 runs): " << t_seq << " s\n";
  }

  std::cout << "Speculative parallel: valid coloring? " << (spec_ok ? "yes" : "no") << "\n";
  if (!spec_ok && spec_conflict.has_value()) {
    const std::int32_t u = spec_conflict->first;
    const std::int32_t v = spec_conflict->second;
    std::cout << "  (invalid: edge/arc in CSR with same color — u=" << u << " v=" << v
              << ", color=" << spec_last.color[static_cast<std::size_t>(u)] << ")\n";
  }
  std::cout << "  colors used: " << spec_last.num_colors << "\n";
  std::cout << "  rounds: " << spec_last.rounds << "\n";
  {
    std::cout << std::fixed << std::setprecision(9);
    std::cout << "  T_spec (median of 5 runs): " << t_spec << " s\n";
  }

  std::cout << "Jones-Plassman: valid coloring? " << (jp_ok ? "yes" : "no") << "\n";
  if (!jp_ok && jp_conflict.has_value()) {
    const std::int32_t u = jp_conflict->first;
    const std::int32_t v = jp_conflict->second;
    std::cout << "  (invalid: edge/arc in CSR with same color — u=" << u << " v=" << v
              << ", color=" << jp_last.color[static_cast<std::size_t>(u)] << ")\n";
  }
  std::cout << "  colors used: " << jp_last.num_colors << "\n";
  std::cout << "  rounds: " << jp_last.rounds << "\n";
  {
    std::cout << std::fixed << std::setprecision(9);
    std::cout << "  T_jp (median of 5 runs): " << t_jp << " s\n";
  }
  const double tick = omp_get_wtick();
  if (t_seq < tick) {
    std::cout << "  (note: median is below omp_get_wtick() ≈ " << tick
              << " s; use a larger graph for a meaningful T_seq)\n";
  }

  return (seq_ok && spec_ok && jp_ok) ? 0 : 2;
}
