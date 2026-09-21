#include "jev_config.hpp"

#include "duckdb/main/client_context.hpp"
#include "duckdb/main/database.hpp"

#include <cstdlib>

namespace duckdb {

void JevConfig::RegisterSettings(DBConfig &config) {
	auto add = [&](const char *name, const char *description, LogicalType type, Value default_value) {
		if (config.HasExtensionOption(name)) {
			return;
		}
		config.AddExtensionOption(name, description, std::move(type), std::move(default_value));
	};

	add("jev_api_key", "TypeSafe API key. Falls back to the TYPESAFE_API_KEY environment variable",
	    LogicalType::VARCHAR, Value());
	add("jev_api_url", "Endpoint that answers the questions (proxies, mocks, self-hosted)", LogicalType::VARCHAR,
	    Value("https://api.typesafe.ai/v1/systemone"));
	add("jev_model", "Model name, or a pinned version such as jev-1.13.0", LogicalType::VARCHAR, Value("jev-latest"));
	add("jev_threshold", "Probability at which jev() returns true", LogicalType::DOUBLE, Value::DOUBLE(0.5));
	add("jev_batch_size", "Rows per API request. Accuracy drops measurably above ~20-25", LogicalType::UBIGINT,
	    Value::UBIGINT(20));
	add("jev_concurrency", "Requests in flight at once, across all DuckDB threads", LogicalType::UBIGINT,
	    Value::UBIGINT(16));
	add("jev_timeout", "Seconds a single API request may take", LogicalType::DOUBLE, Value::DOUBLE(30.0));
	add("jev_max_retries", "Attempts for a retryable failure (429, 5xx, dropped connection)", LogicalType::UBIGINT,
	    Value::UBIGINT(6));
	add("jev_max_rows_per_statement", "Refuse to send more rows than this per statement. 0 = no limit",
	    LogicalType::UBIGINT, Value::UBIGINT(0));
	add("jev_max_chars_per_statement",
	    "Refuse to send more characters of row data than this per statement. 0 = no limit", LogicalType::UBIGINT,
	    Value::UBIGINT(0));
	add("jev_cache_max_entries", "Answers kept in the session cache before the oldest are dropped",
	    LogicalType::UBIGINT, Value::UBIGINT(200000));
}

static bool TryGet(ClientContext &context, const char *name, Value &result) {
	if (!context.TryGetCurrentSetting(name, result)) {
		return false;
	}
	return !result.IsNull();
}

JevConfig JevConfig::FromContext(ClientContext &context) {
	JevConfig config;
	Value value;

	if (TryGet(context, "jev_api_key", value) && !value.ToString().empty()) {
		config.api_key = value.ToString();
	} else {
		auto from_env = std::getenv("TYPESAFE_API_KEY");
		config.api_key = from_env ? string(from_env) : string();
	}
	if (TryGet(context, "jev_api_url", value) && !value.ToString().empty()) {
		config.api_url = value.ToString();
	}
	if (TryGet(context, "jev_model", value) && !value.ToString().empty()) {
		config.model = value.ToString();
	}
	if (TryGet(context, "jev_threshold", value)) {
		config.threshold = value.GetValue<double>();
	}
	if (TryGet(context, "jev_batch_size", value)) {
		config.batch_size = MaxValue<idx_t>(1, value.GetValue<uint64_t>());
	}
	if (TryGet(context, "jev_concurrency", value)) {
		config.concurrency = MaxValue<idx_t>(1, value.GetValue<uint64_t>());
	}
	if (TryGet(context, "jev_timeout", value)) {
		config.timeout = MaxValue<double>(0.1, value.GetValue<double>());
	}
	if (TryGet(context, "jev_max_retries", value)) {
		config.max_retries = MaxValue<idx_t>(1, value.GetValue<uint64_t>());
	}
	if (TryGet(context, "jev_max_rows_per_statement", value)) {
		config.max_rows_per_statement = value.GetValue<uint64_t>();
	}
	if (TryGet(context, "jev_max_chars_per_statement", value)) {
		config.max_chars_per_statement = value.GetValue<uint64_t>();
	}
	if (TryGet(context, "jev_cache_max_entries", value)) {
		config.cache_max_entries = value.GetValue<uint64_t>();
	}
	return config;
}

void JevConfig::RequireAPIKey() const {
	if (api_key.empty()) {
		throw InvalidInputException("jev: no API key. SET jev_api_key = '...' or start DuckDB with "
		                            "TYPESAFE_API_KEY set in the environment.");
	}
}

} // namespace duckdb
