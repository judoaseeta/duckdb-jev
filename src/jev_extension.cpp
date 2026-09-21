#define DUCKDB_EXTENSION_MAIN

#include "jev_extension.hpp"

#include "jev_client.hpp"
#include "jev_config.hpp"
#include "jev_functions.hpp"

#include "duckdb.hpp"
#include "duckdb/main/database.hpp"

namespace duckdb {

static void LoadInternal(ExtensionLoader &loader) {
	loader.SetDescription("Ask your DuckDB tables questions in plain language");
	JevConfig::RegisterSettings(DBConfig::GetConfig(loader.GetDatabaseInstance()));
	JevRegisterFunctions(loader);
}

void JevExtension::Load(ExtensionLoader &loader) {
	LoadInternal(loader);
}

std::string JevExtension::Name() {
	return "jev";
}

std::string JevExtension::Version() const {
#ifdef EXT_VERSION_JEV
	return EXT_VERSION_JEV;
#else
	return JEV_VERSION;
#endif
}

} // namespace duckdb

extern "C" {

DUCKDB_CPP_EXTENSION_ENTRY(jev, loader) {
	duckdb::LoadInternal(loader);
}
}
