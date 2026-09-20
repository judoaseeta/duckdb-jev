#!/usr/bin/env bash
# Re-vendors the two header-only dependencies into third_party/.
#
# cpp-httplib is vendored as `jev_httplib.hpp` in the namespace `duckdb_jev_httplib`:
# DuckDB itself ships third_party/httplib/httplib.hpp on the include path ahead of
# ours, so the file name has to differ, and two copies of `namespace httplib` in one
# process would resolve to whichever loaded first. DuckDB renames its copy the same way.
set -euo pipefail

HTTPLIB_VERSION="v0.20.0"
JSON_VERSION="v3.11.3"

here="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
mkdir -p "$here/third_party/httplib" "$here/third_party/json"

curl -fsSL "https://raw.githubusercontent.com/yhirose/cpp-httplib/${HTTPLIB_VERSION}/httplib.h" \
  -o "$here/third_party/httplib/jev_httplib.hpp"
sed -i.bak \
  -e 's/^namespace httplib {/namespace duckdb_jev_httplib {/' \
  -e 's/httplib::status_message/duckdb_jev_httplib::status_message/' \
  -e 's/CPPHTTPLIB_HTTPLIB_H/JEV_CPPHTTPLIB_HTTPLIB_H/g' \
  "$here/third_party/httplib/jev_httplib.hpp"
rm -f "$here/third_party/httplib/jev_httplib.hpp.bak"

curl -fsSL "https://raw.githubusercontent.com/nlohmann/json/${JSON_VERSION}/single_include/nlohmann/json.hpp" \
  -o "$here/third_party/json/json.hpp"

grep -q 'namespace duckdb_jev_httplib {' "$here/third_party/httplib/jev_httplib.hpp"
! grep -q '^namespace httplib {' "$here/third_party/httplib/jev_httplib.hpp"
echo "vendored cpp-httplib ${HTTPLIB_VERSION} (namespace duckdb_jev_httplib) and nlohmann/json ${JSON_VERSION}"
