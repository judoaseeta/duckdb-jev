#pragma once

#include "duckdb.hpp"
#include "duckdb/main/extension/extension_loader.hpp"

namespace duckdb {

//! Registers jev(), jev_prob(), jev_score(), jev_score_norm(), jev_choice(),
//! jev_confidence(), jev_eval() and the session helpers.
void JevRegisterFunctions(ExtensionLoader &loader);

} // namespace duckdb
