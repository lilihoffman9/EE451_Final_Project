#!/usr/bin/env bash
# Download SNAP edge lists used in the EE451 proposal (uncompress to .txt).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DATA="${ROOT}/data"
mkdir -p "${DATA}"

fetch() {
  local name="$1"
  local url="$2"
  local out="${DATA}/${name}.txt"
  if [[ -f "${out}" ]]; then
    echo "Already have ${out}"
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

echo "Done. Example: ./bin/coloring ${DATA}/email-EuAll.txt 8"
