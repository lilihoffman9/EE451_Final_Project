#include "graph_csr.hpp"
#include "greedy_sequential.hpp"
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
  GreedySequentialStats last;
  const double t_seq =
      median_wall_seconds([&]() { last = greedy_sequential_first_fit(g); }, 5);
  const bool ok = validate_coloring(g, last.color);

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

  std::cout << "Sequential greedy: valid coloring? " << (ok ? "yes" : "no") << "\n";
  std::cout << "  colors used: " << last.num_colors << "\n";
  {
    std::cout << std::fixed << std::setprecision(9);
    std::cout << "  T_seq (median of 5 runs): " << t_seq << " s\n";
  }
  const double tick = omp_get_wtick();
  if (t_seq < tick) {
    std::cout << "  (note: median is below omp_get_wtick() ≈ " << tick
              << " s; use a larger graph for a meaningful T_seq)\n";
  }

  return ok ? 0 : 2;
}
