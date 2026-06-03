# Version Tag Reference

| Tag | Commit | Date | Description |
|-----|--------|------|-------------|
| `v0.0.9` | `145518c` | 2026-06-02 | Bug fixes: tile tearing, wrap, ranged AI, magic def, map close, HP scaling, ranged weapons |
| `v0.0.8` | `801114e` | 2026-06-02 | Refactoring: game_balance.h, equipment_bonus, floor_init, unit tests, validation, profiling, API docs |

## Mapping tags to changelog entries

- **v0.0.9** → [changelog.md](./changelog.md#v009---2026-06-02)
- **v0.0.8** → [changelog.md](./changelog.md#latest---2026-06-02) (Codebase Refactoring)
- **ALPHA-0.0.7 and earlier** → [HISTORY.md](../HISTORY.md)

## Creating a new version tag

```sh
git tag -a v0.1.0 -m "v0.1.0: short description"
git push origin v0.1.0
```
