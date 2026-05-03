# EE451: Parallel and Distributed Computing - Graph Coloring Project

## Overview
This project implements and evaluates three graph coloring algorithms:
- Sequential Greedy Coloring (baseline)
- Speculative Parallel Coloring
- Jones-Plassman Parallel Coloring

All algorithms use a shared CSR graph representation and are implemented in C++ with OpenMP for parallelism.

## Project Structure
- `algorithms/`: Coloring algorithm implementations
- `supportingFiles/include/`: Headers (graph_csr.hpp, timing.hpp)
- `supportingFiles/src/`: Source files (main.cpp, load_snap.cpp, validate.cpp)
- `supportingFiles/fixtures/`: Small test graph (tiny_edges.txt)
- `supportingFiles/scripts/`: Benchmark scripts and dataset downloader
- `Makefile`: Build configuration

## Prerequisites
- **Linux/Mac**: GCC with OpenMP support
  - Ubuntu/Debian: `sudo apt install build-essential libomp-dev`
  - macOS: `brew install libomp`
- **Windows**: Use WSL (see Windows instructions below)

## Building
```bash
cd /path/to/project
make
```
This creates the binary at `bin/coloring`.

## Testing

### Quick Test (Small Graph)
```bash
./bin/coloring supportingFiles/fixtures/tiny_edges.txt
```
- Runs all three algorithms
- Outputs validity, colors used, rounds, and median execution times

### Download Datasets
```bash
./supportingFiles/scripts/download_snap_datasets.sh
```
Downloads four SNAP graphs to `data/` (or `$EE451_DATA_DIR` if set).

### Run Benchmarks
- Sequential baseline on all datasets:
  ```bash
  ./supportingFiles/scripts/bench_sequential_all.sh
  ```
- Parallel algorithms with specific threads:
  ```bash
  OMP_NUM_THREADS=4 ./bin/coloring data/email-EuAll.txt
  ```
- Vary `OMP_NUM_THREADS` (1, 2, 4, 8, ...) to measure speedup and scalability

### Evaluation Metrics
- **Correctness**: All algorithms should show "valid coloring? yes"
- **Performance**: Wall-clock time (median of 5 runs)
- **Speedup**: T_sequential / T_parallel
- **Scalability**: Speedup / number_of_threads
- **Colors Used**: Parallel may use slightly more than sequential
- **Rounds**: Speculative (1-3 typically), Jones-Plassman (O(log n))

## Windows Setup (WSL)
1. Install WSL and Ubuntu from Microsoft Store
2. In WSL terminal:
   ```bash
   sudo apt update
   sudo apt install build-essential libomp-dev wget unzip python3
   ```
3. Copy project: `cp -r /mnt/c/Users/.../EE451_Final_Project ~`
4. Follow Linux/Mac instructions above

## Troubleshooting
- Build fails: Ensure GCC and OpenMP are installed
- OpenMP not found: Check `gcc --version` and install libomp-dev
- Datasets not downloading: Check internet; script uses wget
- Performance issues: Verify `OMP_NUM_THREADS` is set correctly

## Authors
- Lili Hoffman, Anoushka Narayan, Kayal Bhatia</content>