#pragma once

#include "duckdb.hpp"

namespace duckdb {

//! Every jev_* setting, resolved for one query.
//!
//! Settings are read once per bind (i.e. per query), so `SET jev_batch_size = 40`
//! takes effect on the next statement and never changes mid-scan.
struct JevConfig {
	string api_key;
	string api_url = "https://api.typesafe.ai/v1/systemone";
	string model = "jev-latest";
	double threshold = 0.5;
	idx_t batch_size = 20;
	idx_t concurrency = 16;
	double timeout = 30.0;
	idx_t max_retries = 6;
	idx_t max_rows_per_statement = 0;
	idx_t max_chars_per_statement = 0;
	idx_t cache_max_entries = 200000;

	//! Registers the settings on the database config. Called once, when the extension loads.
	static void RegisterSettings(DBConfig &config);
	//! Resolves the settings as they stand for this query. The API key falls back to
	//! the TYPESAFE_API_KEY environment variable, like pg-jev does.
	static JevConfig FromContext(ClientContext &context);
	//! Throws if no API key was configured. Called when a request is about to be sent,
	//! never at bind time, so a query answered entirely from cache needs no key.
	void RequireAPIKey() const;
};

} // namespace duckdb
