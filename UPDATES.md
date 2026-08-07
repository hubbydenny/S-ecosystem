# UPDATES

## [0.2.0] — 2026-08-07
### Added
- toml++ config: auto-generated `config.toml`
- 11 `[setup]` toggles, `[colors] logocolor`, `[state].lastrun`

### Fixed
- duplicate `logo = true` broke parsing
- `kermel` → `kernel`, `loadconfig` → `loadConfig`
- `n = "white"` → `n == "white"`
- `getGPUModel()` missing return
