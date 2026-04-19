#!/usr/bin/env bash
# Run sequential greedy on all four SNAP graphs (after download_snap_datasets.sh).
# Invoke from repo root; script lives under supportingFiles/scripts/.
#
# Graphs are read from EE451_DATA_DIR if set, else <repo>/data. Match download_snap_datasets.sh.
#
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DATA="${EE451_DATA_DIR:-${ROOT}/data}"
BIN="${ROOT}/bin/coloring"
THREADS="${THREADS:-8}"

if [[ ! -x "${BIN}" ]]; then
  echo "Build first: (cd \"${ROOT}\" && make)" >&2
  exit 1
fi

echo "================================================================================"
echo "Sequential greedy baseline  |  OMP_NUM_THREADS=${THREADS}  |  T_seq = median of 5 runs"
echo "================================================================================"
echo ""
printf '%-28s %12s %14s %8s %8s %16s\n' "dataset" "n" "m" "valid" "colors" "T_seq_s"
echo "--------------------------------------------------------------------------------"

for f in \
  "${DATA}/email-EuAll.txt" \
  "${DATA}/com-Youtube.txt" \
  "${DATA}/com-LiveJournal.txt" \
  "${DATA}/roadNet-CA.txt"; do
  base="$(basename "${f}")"
  if [[ ! -f "${f}" ]]; then
    printf '%-28s %12s %14s %8s %8s %16s\n' "${base}" "-" "-" "skip" "-" "-"
    continue
  fi
  echo "  … running ${base}" >&2
  OMP_NUM_THREADS="${THREADS}" "${BIN}" "${f}" "${THREADS}" --brief
done

echo ""
echo "Tip: one graph with full details —  ./bin/coloring \"${DATA}/email-EuAll.txt\" ${THREADS}"
