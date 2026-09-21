# Tests

`sql/` holds [SQLLogicTests](https://duckdb.org/dev/sqllogictest/intro.html):

- `jev_offline.test` — everything that needs no API: the settings and their defaults, NULL handling,
  the argument validation, the spend guard, and what an unreachable endpoint looks like.
- `jev_api.test` — the judgment functions against `mock_api.py`. Skipped unless `JEV_MOCK_API_URL`
  is set, which `scripts/run_tests_with_mock.sh` does for you.

```bash
make test        # the offline tests; jev_api.test is skipped
make test_mock   # starts mock_api.py and runs everything
```

`mock_api.py` is a deterministic stand-in for the TypeSafe endpoint, so the expected results never
move and no test ever reaches the live API:

- `noul` answers 0.9 when the last word of the condition occurs in the row JSON, else 0.1
- `score` picks the level at `len(row_json) % len(levels)`
- `choice` picks the option at `len(row_json) % len(options)`
- a condition containing `trigger422` answers 422, `trigger503` answers 503

To try the real API, `SET jev_api_key` in a normal DuckDB session and run a query.
