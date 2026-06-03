# Test Suite — Heroes of Taboo

## Running Tests

```sh
cd game
make test          # build + run all tests
make config=debug_x64 test  # same, explicit config
```

Requires a working `gcc` and `raylib` build in `bin/Debug/`.

## Test Structure

Tests live in a single file: `tests/test_runner.c`. The Makefile `test` target links
`test_runner.o` against all game `.o` files (excluding `main.o`) and runs the binary.

## Adding New Tests

1. Write a `static bool test_my_feature(void)` function in `tests/test_runner.c`
2. Register it in the `g_tests[]` array at the bottom of the file
3. Run `make test` to verify

### Assertion helpers

```c
CHECK(condition, "message");            // fail + print message if condition false
CHECK_EQ(a, b, "message");              // fail + print actual/expected if a != b
TEST_PASS();                             // mark test as passed
TEST_FAIL("message");                    // fail immediately
```

## Test Categories

| Category | Count | What it covers |
|----------|-------|----------------|
| Formulas | 11 | XP, HP, melee/ranged/throw/magic damage, dodge, throw range, wait heal, potion heal |
| Inventory data | 2 | Equipment table bounds + values, item names + heal amounts |
| Equipment logic | 3 | Apply/remove/recalculate stat bonuses, edge cases (NULL, missing CStats) |
| Validation | 7 | Slot, equip type, item type, monster type, stat index, floor, clamp |
| Spatial / perf | 5 | Spatial hash add/remove/move/exclude, alive monster counter, grid init |

Total: **28 tests** — all deterministic, no external dependencies.
