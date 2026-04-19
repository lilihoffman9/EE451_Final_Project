#include "graph_csr.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

void trim_comment(std::string &line) {
  const auto hash = line.find('#');
  if (hash != std::string::npos) {
    line.erase(hash);
  }
  while (!line.empty() && std::isspace(static_cast<unsigned char>(line.back()))) {
    line.pop_back();
  }
}

bool parse_two_ints(const std::string &line, std::int64_t &a, std::int64_t &b) {
  std::istringstream iss(line);
  if (!(iss >> a >> b)) {
    return false;
  }
  char extra = 0;
  if (iss >> extra) {
    return false;
  }
  return true;
}

}  // namespace

GraphCSR load_snap_edge_list(const std::string &path) {
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("Cannot open graph file: " + path);
  }

  std::int64_t vmin = std::numeric_limits<std::int64_t>::max();
  std::int64_t vmax = std::numeric_limits<std::int64_t>::min();

  std::string line;
  while (std::getline(in, line)) {
    trim_comment(line);
    if (line.empty()) {
      continue;
    }
    std::int64_t u = 0, v = 0;
    if (!parse_two_ints(line, u, v)) {
      continue;
    }
    if (u == v) {
      continue;
    }
    vmin = std::min(vmin, std::min(u, v));
    vmax = std::max(vmax, std::max(u, v));
  }

  if (vmin == std::numeric_limits<std::int64_t>::max()) {
    throw std::runtime_error("No valid edges found in: " + path);
  }

  const std::int64_t span = vmax - vmin + 1;
  if (span > static_cast<std::int64_t>(1) << 30) {
    throw std::runtime_error("Vertex id span too large for 32-bit CSR indexing; add id remapping.");
  }

  const std::int32_t n = static_cast<std::int32_t>(span);
  std::vector<std::int64_t> deg(static_cast<std::size_t>(n), 0);

  in.clear();
  in.seekg(0);
  while (std::getline(in, line)) {
    trim_comment(line);
    if (line.empty()) {
      continue;
    }
    std::int64_t u = 0, v = 0;
    if (!parse_two_ints(line, u, v)) {
      continue;
    }
    if (u == v) {
      continue;
    }
    const std::int32_t ui = static_cast<std::int32_t>(u - vmin);
    const std::int32_t vi = static_cast<std::int32_t>(v - vmin);
    ++deg[static_cast<std::size_t>(ui)];
    ++deg[static_cast<std::size_t>(vi)];
  }

  GraphCSR g;
  g.n = n;
  g.row_ptr.assign(static_cast<std::size_t>(n) + 1, 0);
  for (std::int32_t i = 0; i < n; ++i) {
    g.row_ptr[static_cast<std::size_t>(i) + 1] = g.row_ptr[static_cast<std::size_t>(i)] + deg[static_cast<std::size_t>(i)];
  }
  const std::int64_t nnz = g.row_ptr[static_cast<std::size_t>(n)];
  g.col_idx.resize(static_cast<std::size_t>(nnz));
  std::vector<std::int64_t> next = g.row_ptr;

  in.clear();
  in.seekg(0);
  while (std::getline(in, line)) {
    trim_comment(line);
    if (line.empty()) {
      continue;
    }
    std::int64_t u = 0, v = 0;
    if (!parse_two_ints(line, u, v)) {
      continue;
    }
    if (u == v) {
      continue;
    }
    const std::int32_t ui = static_cast<std::int32_t>(u - vmin);
    const std::int32_t vi = static_cast<std::int32_t>(v - vmin);
    const std::size_t pos_u = static_cast<std::size_t>(next[static_cast<std::size_t>(ui)]++);
    const std::size_t pos_v = static_cast<std::size_t>(next[static_cast<std::size_t>(vi)]++);
    g.col_idx[pos_u] = vi;
    g.col_idx[pos_v] = ui;
  }

  std::vector<std::int32_t> new_col;
  new_col.reserve(static_cast<std::size_t>(nnz));
  std::vector<std::int64_t> new_row(static_cast<std::size_t>(n) + 1, 0);

  for (std::int32_t u = 0; u < n; ++u) {
    const std::int64_t beg = g.row_ptr[static_cast<std::size_t>(u)];
    const std::int64_t end = g.row_ptr[static_cast<std::size_t>(u) + 1];
    std::vector<std::int32_t> neigh(static_cast<std::size_t>(end - beg));
    for (std::int64_t k = beg; k < end; ++k) {
      neigh[static_cast<std::size_t>(k - beg)] = g.col_idx[static_cast<std::size_t>(k)];
    }
    std::sort(neigh.begin(), neigh.end());
    neigh.erase(std::unique(neigh.begin(), neigh.end()), neigh.end());
    new_col.insert(new_col.end(), neigh.begin(), neigh.end());
    new_row[static_cast<std::size_t>(u) + 1] = static_cast<std::int64_t>(new_col.size());
  }

  g.col_idx = std::move(new_col);
  g.row_ptr = std::move(new_row);
  g.num_undirected_edges = g.row_ptr[static_cast<std::size_t>(n)] / 2;
  return g;
}
