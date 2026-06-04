# Tech Stack

- Core language: C++ engine, C/C++ third-party libs, Python build/generation scripts, XML class docs.
- Build: SCons (`SConstruct`, `SCsub`), minimum SCons 4.0 and Python 3.8. Optional SCons ninja backend requires SCons 4.2+.
- Tests: doctest vendored under `thirdparty/doctest`, compiled into engine when `tests=yes`.
- Format/lint configs: `.clang-format`, `.clang-tidy`, `pyproject.toml` for ruff/mypy/codespell.
- Version metadata: `version.py` currently Godot 4.6.3 stable, docs branch 4.6.
- Supported arch aliases/options in `platform_methods.py`: x86_32/x86_64/arm32/arm64/rv64/ppc64/wasm32/loongarch64 with aliases like x64/amd64/aarch64/riscv.
- Common generated artifacts: `*.gen.h`, `*.gen.cpp`, `compile_commands.json`, `build.ninja`, `bin/` outputs.
- Platform target examples from repo: `linuxbsd`, `windows`, `macos`, `android`, `ios`, `web`, `visionos`.