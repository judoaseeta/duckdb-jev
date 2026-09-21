#pragma once

#include "duckdb.hpp"

namespace duckdb {

//! Serialises a DuckDB value to JSON the way `to_json` would: struct field names and
//! their declared order are preserved, because the column names are part of what the
//! model reads. Used for the row payload and for the cache key.
string JevValueToJSON(const Value &value);

//! Appends `text` to `out` as a quoted JSON string.
void JevWriteJSONString(const string &text, string &out);

//! `text` as a quoted JSON string.
string JevQuoteJSONString(const string &text);

} // namespace duckdb
