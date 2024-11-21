#!/bin/sh

export LC_ALL=C.UTF-8

set -eu

if ! command -v go > /dev/null
then
  echo "Generating the RPC documentation requires 'go' to be installed"
  exit 1
fi

GENERATOR_SCRIPT="$1"
DOGECASHD_COMMAND="$2"
DOGECASH_CLI_COMMAND="$3"

DOGECASHD_PID_FILE="_dogecashd.pid"
"${DOGECASHD_COMMAND}" -regtest -daemon -pid="${DOGECASHD_PID_FILE}"

shutdown_dogecashd() {
  "${DOGECASH_CLI_COMMAND}" -regtest stop > /dev/null 2>&1

  # Waiting for dogecashd shut down
  PID_WAIT_COUNT=0
  while [ -e "${DOGECASHD_PID_FILE}" ]
  do
    : $((PID_WAIT_COUNT+=1))
    if [ "${PID_WAIT_COUNT}" -gt 20 ]
    then
      echo "Timed out waiting for dogecashd to stop"
      exit 3
    fi
    sleep 0.5
  done
}
trap "shutdown_dogecashd" EXIT

# Waiting for dogecashd to spin up
RPC_HELP_WAIT_COUNT=0
while ! "${DOGECASH_CLI_COMMAND}" -regtest help > /dev/null 2>&1
do
  : $((RPC_HELP_WAIT_COUNT+=1))
  if [ "${RPC_HELP_WAIT_COUNT}" -gt 10 ]
  then
    echo "Timed out waiting for dogecashd to start"
    exit 2
  fi
  sleep 0.5
done

go run "${GENERATOR_SCRIPT}" -regtest
