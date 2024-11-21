#!/usr/bin/env bash

export LC_ALL=C

set -euxo pipefail

# Do not leave any dangling subprocesses when this script exits
trap "kill 0" SIGINT

TOPLEVEL=$(git rev-parse --show-toplevel)
DEFAULT_DOGECASHD="${TOPLEVEL}/build/src/dogecashd"
DEFAULT_LOG_FILE=~/".dogecash/debug.log"

help_message() {
  set +x
  echo "Run dogecashd until a given log message is encountered, then kill dogecashd."
  echo ""
  echo "Example usages:"
  echo "$0 --grep 'progress=1.000000' --params \"-datadir=~/.dogecash\" --callback mycallback"
  echo ""
  echo "Options:"
  echo "-h, --help            Display this help message."
  echo ""
  echo "-g, --grep            (required) The grep pattern to look for."
  echo ""
  echo "-c, --callback        (optional) Bash command to execute as a callback. This is useful for interacting with dogecashd before it is killed (to run tests, for example)."
  echo "-p, --params          (optional) Parameters to provide to dogecashd."
  echo ""
  echo "Environment Variables:"
  echo "DOGECASHD              Default: ${DEFAULT_DOGECASHD}"
  echo "LOG_FILE              Default: ${DEFAULT_LOG_FILE}"
  set -x
}

: "${DOGECASHD:=${DEFAULT_DOGECASHD}}"
: "${LOG_FILE:=${DEFAULT_LOG_FILE}}"
GREP_PATTERN=""
CALLBACK=""
DOGECASHD_PARAMS=""

# Parse command line arguments
while [[ $# -gt 0 ]]; do
case $1 in
  -h|--help)
    help_message
    exit 0
    ;;

  # Required params
  -g|--grep)
    GREP_PATTERN="$2"
    shift # shift past argument
    shift # shift past value
    ;;

  # Optional params
  -c|--callback)
    CALLBACK="$2"
    shift # shift past argument
    shift # shift past value
    ;;
  -p|--params)
    DOGECASHD_PARAMS="$2"
    shift # shift past argument
    shift # shift past value
    ;;

  *)
    echo "Unknown argument: $1"
    help_message
    exit 1
    ;;
esac
done

if [ -z "${GREP_PATTERN}" ]; then
  echo "Error: A grep pattern was not specified."
  echo ""
  help_message
  exit 2
fi

# Make sure the debug log exists so that tail does not fail
touch "${LOG_FILE}"

# Launch dogecashd using custom parameters
read -a DOGECASHD_PARAMS <<< "${DOGECASHD_PARAMS}"

DOGECASHD_PID_FILE=/tmp/dogecashd-exit-on-log.pid
# Make sure the PID file doesn't already exist for some reason
rm -f "${DOGECASHD_PID_FILE}"
DOGECASHD_PARAMS+=("-pid=${DOGECASHD_PID_FILE}")
DOGECASHD_PARAMS+=("-daemon")

START_TIME=$(date +%s)
"${DOGECASHD}" "${DOGECASHD_PARAMS[@]}"

# The PID file will not exist immediately, so wait for it
PID_WAIT_COUNT=0
while [ ! -e "${DOGECASHD_PID_FILE}" ]; do
  ((PID_WAIT_COUNT+=1))
  if [ "${PID_WAIT_COUNT}" -gt 10 ]; then
    echo "Timed out waiting for dogecashd PID file"
    exit 10
  fi
  sleep 0.5
done
DOGECASHD_PID=$(cat "${DOGECASHD_PID_FILE}")

# Wait for log checking to finish and kill the daemon
(
  # When this subshell finishes, kill dogecashd
  # shellcheck disable=SC2317
  log_subshell_cleanup() {
    echo "Cleaning up bitcoin daemon (PID: ${DOGECASHD_PID})."
    kill ${DOGECASHD_PID}
  }
  trap "log_subshell_cleanup" EXIT

  (
    # Ignore the broken pipe when tail tries to write pipe closed by grep
    set +o pipefail
    tail -f "${LOG_FILE}" | grep -m 1 "${GREP_PATTERN}"
  )

  END_TIME=$(date +%s)
  RUNTIME=$((END_TIME - START_TIME))
  HUMAN_RUNTIME=$(eval "echo $(date -ud "@${RUNTIME}" "+\$((%s/3600)) hours, %M minutes, %S seconds")")

  echo "Grep pattern '${GREP_PATTERN}' found after ${HUMAN_RUNTIME}."

  # Optional callback for interacting with dogecashd before it's killed
  if [ -n "${CALLBACK}" ]; then
    "${CALLBACK}"
  fi
) &
LOG_PID=$!

# Wait for dogecashd to exit, whether it exited on its own or the log subshell finished
set +x
while [ -e "${DOGECASHD_PID_FILE}" ]; do sleep 0.5; done
set -x

# If the log subshell is still running, then GREP_PATTERN was not found
if [ -e /proc/${LOG_PID} ]; then
  echo "dogecashd exited unexpectedly. See '${LOG_FILE}' for details."
  exit 20
fi

# Get the exit code for the log subshell, which should have exited already
# if GREP_PATTERN was found
wait ${LOG_PID}
LOG_EXIT_CODE=$?

# The log subshell should only exit with a non-zero code if the callback
# failed.
if [ "${LOG_EXIT_CODE}" -ne "0" ]; then
  echo "Log subshell failed with code: ${LOG_EXIT_CODE}"
  exit ${LOG_EXIT_CODE}
fi
