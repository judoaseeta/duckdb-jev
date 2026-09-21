#!/usr/bin/env bash
# Runs the regression suite with the deterministic mock API in front of it.
#
# TYPESAFE_API_KEY is dropped on purpose: the offline test asserts what happens when
# no key is configured, and a developer's real key in the environment would turn that
# into a live request.
set -euo pipefail

BUILD=${BUILD:-release}
PORT=${JEV_MOCK_PORT:-18765}
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

unittest="./build/${BUILD}/test/unittest"
if [ ! -x "$unittest" ]; then
  echo "no test runner at $unittest - run 'make ${BUILD}' first" >&2
  exit 1
fi

python3 test/mock_api.py "$PORT" &
mock_pid=$!
trap 'kill ${mock_pid} 2>/dev/null || true' EXIT

for _ in $(seq 1 50); do
  if ! kill -0 "${mock_pid}" 2>/dev/null; then
    # Almost always "address already in use": something else owns the port, and
    # running the suite against it would test that something else.
    echo "the mock API did not start on port ${PORT} - set JEV_MOCK_PORT to a free port" >&2
    exit 1
  fi
  if python3 -c "import socket,sys; socket.create_connection(('127.0.0.1', ${PORT}), 0.2).close()" 2>/dev/null; then
    break
  fi
  sleep 0.1
done

export JEV_MOCK_API_URL="http://127.0.0.1:${PORT}/v1/systemone"
export JEV_ASSERT_NO_API_KEY=1
env -u TYPESAFE_API_KEY "$unittest" --test-dir . "${1:-test/*}"
