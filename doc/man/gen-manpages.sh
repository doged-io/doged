#!/usr/bin/env bash
# Copyright (c) 2020 The Bitcoin developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C

set -euxo pipefail

usage() {
  cat << EOF
Usage: $0 dogecashd binary manpage
  dogecashd: path to dogecashd executable
  binary: path to the binary to generate the man pages from
  manpage: output path for the man page
EOF
}

if [ $# -ne 3 ]
then
  usage
  exit 1
fi

if ! command -v help2man
then
  echo "help2man is required to run $0, please install it"
  exit 2
fi

DOGECASHD="$1"
BIN="$2"
MANPAGE="$3"

if [ ! -x "${DOGECASHD}" ]
then
  echo "${DOGECASHD} not found or not executable."
  exit 4
fi
if [ ! -x "${BIN}" ]
then
  echo "${BIN} not found or not executable."
  exit 5
fi

mkdir -p "$(dirname ${MANPAGE})"

# The autodetected version git tag can screw up manpage output a little bit
read -r -a VERSION <<< "$(${DOGECASHD} --version | awk -F'[ -]' 'NR == 1 { print $4, $5 }')"

# Create a footer file with copyright content.
# This gets autodetected fine for dogecashd if --version-string is not set,
# but has different outcomes for dogecash-qt and dogecash-cli.
FOOTER="$(basename ${BIN})_footer.h2m"
cleanup() {
  rm -f "${FOOTER}"
}
trap "cleanup" EXIT
echo "[COPYRIGHT]" > "${FOOTER}"
"${DOGECASHD}" --version | sed -n '1!p' >> "${FOOTER}"

help2man -N --version-string="${VERSION[0]}" --include="${FOOTER}" -o "${MANPAGE}" "${BIN}"
sed -i "s/\\\-${VERSION[1]}//g" "${MANPAGE}"
