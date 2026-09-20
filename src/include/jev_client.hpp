#pragma once

#include "duckdb.hpp"
#include "jev_config.hpp"

namespace duckdb {

//! Extension version, reported by jev_version() and sent as the User-Agent.
#define JEV_VERSION "0.1.0"

//! One question, asked of every row in a batch.
struct JevQuestion {
	//! "noul" (yes/no probability), "score" (ordered levels) or "choice" (one of n)
	string kind;
	//! The condition or question, in plain language
	string query;
	//! Levels for "score", options for "choice"; empty for "noul"
	vector<string> options;

	//! Identifies this question for the answer cache. Two calls with the same
	//! (kind, query, options) share cached answers, so jev_choice() and
	//! jev_confidence() on the same arguments cost one request, not two.
	string CacheKey() const;
};

//! Judges one batch of rows: one request, one shared state, one question per row.
//! Returns the raw answer JSON per row, in the order the rows were given.
//! Retries the retryable failures (429, 5xx, dropped connections) and throws otherwise.
vector<string> JevCallAPI(const JevConfig &config, const JevQuestion &question, const vector<string> &rows_json);

} // namespace duckdb
