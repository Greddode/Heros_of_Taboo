# Version Tag Reference

| Tag | Commit | Date | Description |
|-----|--------|------|-------------|
| `v0.0.10` | `e82fc51` | 2026-06-03 | Documentation: comprehensive AGENTS.md rewrite with architecture, profiling, patterns, build setup |
| `v0.0.9` | `145518c` | 2026-06-02 | Bug fixes: tile tearing, wrap, ranged AI, magic def, map close, HP scaling, ranged weapons |
| `v0.0.8` | `801114e` | 2026-06-02 | Refactoring: game_balance.h, equipment_bonus, floor_init, unit tests (28), validation, profiling, API docs |

## Mapping tags to changelog entries

- **v0.0.10** → [changelog.md](./changelog.md#v0010---2026-06-03) (AGENTS.md documentation)
- **v0.0.9** → [changelog.md](./changelog.md#v009---2026-06-02) (Bug fixes)
- **v0.0.8** → [changelog.md](./changelog.md#v008---2026-06-02) (Codebase Refactoring)
- **v0.0.7 and earlier** → [HISTORY.md](../HISTORY.md) (Historical)

## Creating a new version tag

```sh
git tag -a v0.1.0 -m "v0.1.0: short description"
git push origin v0.1.0
```

## Release Checklist

Before creating a new version tag:

1. ✅ All 28 unit tests pass: `cd game && make test`
2. ✅ Code compiles without warnings: `make config=debug_x64`
3. ✅ Changelog entry added in `changelog.md`
4. ✅ Version tag exists and pushed to GitHub
5. ✅ VERSIONS.md updated with tag mapping
6. ✅ AGENTS.md up-to-date with current architecture

---

**Current Status**: v0.0.10 ✅ (AGENTS.md comprehensive, all systems documented)
