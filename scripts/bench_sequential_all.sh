#!/usr/bin/env bash
# Run sequential greedy on all four SNAP graphs in data/ (after download_snap_datasets.sh).
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
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
# Column widths match ./bin/coloring --brief (non-CSV) output.
printf '%-28s %12s %14s %8s %8s %16s\n' "dataset" "n" "m" "valid" "colors" "T_seq_s"
echo "--------------------------------------------------------------------------------"

for f in \
  "${ROOT}/data/email-EuAll.txt" \
  "${ROOT}/data/com-Youtube.txt" \
  "${ROOT}/data/com-LiveJournal.txt" \
  "${ROOT}/data/roadNet-CA.txt"; do
  base="$(basename "${f}")"
  if [[ ! -f "${f}" ]]; then
    printf '%s\t%s\t%s\t%s\t%s\t%s\n' "${base}" "-" "-" "skip" "-" "-"
    continue
  fi
  echo "  … running ${base}" >&2
  OMP_NUM_THREADS="${THREADS}" "${BIN}" "${f}" "${THREADS}" --brief
done

echo ""
echo "Tip: one graph with full details —  ./bin/coloring \"${ROOT}/data/email-EuAll.txt\" ${THREADS}"
