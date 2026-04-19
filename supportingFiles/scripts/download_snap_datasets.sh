#!/usr/bin/env bash
# Download SNAP edge lists used in the EE451 proposal (uncompress to .txt).
# Invoke from repo root; script lives under supportingFiles/scripts/.
#
# Behavior:
#   - If a dataset file already exists, it is NOT downloaded again (safe to re-run).
#   - Set EE451_DATA_DIR to store graphs outside the repo on persistent disk, e.g.:
#       export EE451_DATA_DIR=/mnt/ebs/snap
#       ./supportingFiles/scripts/download_snap_datasets.sh
#     Then run benchmarks with the same export (see bench_sequential_all.sh).
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DATA="${EE451_DATA_DIR:-${ROOT}/data}"
mkdir -p "${DATA}"
echo "EE451 datasets directory: ${DATA}"

fetch() {
  local name="$1"
  local url="$2"
  local out="${DATA}/${name}.txt"
  if [[ -f "${out}" ]]; then
    echo "Skipping download (already exists): ${out}"
    return 0
  fi
  echo "Downloading ${url} ..."
  curl -L --fail --retry 3 --retry-delay 2 -o "${out}.gz" "${url}"
  gunzip -f "${out}.gz"
  echo "Wrote ${out}"
}

# Stanford SNAP static URLs (see https://snap.stanford.edu/data/index.html)
fetch "email-EuAll" "https://snap.stanford.edu/data/email-EuAll.txt.gz"
fetch "com-Youtube" "https://snap.stanford.edu/data/bigdata/communities/com-youtube.ungraph.txt.gz"
fetch "com-LiveJournal" "https://snap.stanford.edu/data/bigdata/communities/com-lj.ungraph.txt.gz"
fetch "roadNet-CA" "https://snap.stanford.edu/data/roadNet-CA.txt.gz"

echo "Done. Example: (cd \"${ROOT}\" && ./bin/coloring \"${DATA}/email-EuAll.txt\" 8)"
