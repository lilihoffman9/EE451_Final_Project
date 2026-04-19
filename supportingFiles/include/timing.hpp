#pragma once

#include <algorithm>
#include <cmath>
#include <functional>
#include <omp.h>
#include <vector>

// Wall time in seconds using OpenMP's timer (median of `runs` samples).
inline double median_wall_seconds(const std::function<void()> &fn, int runs = 5) {
  std::vector<double> t;
  t.reserve(static_cast<std::size_t>(runs));
  for (int i = 0; i < runs; ++i) {
    const double t0 = omp_get_wtime();
    fn();
    const double t1 = omp_get_wtime();
    t.push_back(t1 - t0);
  }
  std::nth_element(t.begin(), t.begin() + t.size() / 2, t.end());
  return t[t.size() / 2];
}
