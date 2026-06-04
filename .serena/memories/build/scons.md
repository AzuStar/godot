# SCons Build

- Requires SCons >= 4.0 and Python >= 3.8 (`SConstruct`).
- Main options live in `SConstruct`; custom option files load from `custom.py` and `profile=<path>`.
- Common local build form: `scons platform=linuxbsd target=editor` plus options.
- `dev_mode=yes` defaults: `verbose=yes warnings=extra werror=yes tests=yes strict_checks=yes` unless user overrides each option.
- `production=yes` defaults: static C++ runtime, no debug symbols, platform-specific production defaults, `lto=auto`.
- `tests=yes` compiles `tests/` and emits `TESTS_ENABLED` for main/tests.
- `compiledb=yes` enables SCons `compilation_db` tool and alias `compiledb`, generating `compile_commands.json`.
- `ninja=yes` requires SCons >= 4.2; sets experimental ninja backend and `build.ninja` by default.
- `scu_build=yes` emits `SCU_BUILD_ENABLED`; `scu_limit` caps includes per SCU file. Dev builds use higher default include grouping.
- Build objects can redirect under `bin/obj` via `redirect_build_objects` and `methods.redirect_emitter`.

Module detection/registration:
- Module directory must contain `register_types.h`, `SCsub`, and `config.py` (`methods.is_module`).
- `config.can_build(env, platform)` gates enablement; `config.configure(env)` mutates env; optional `get_doc_classes`, `get_doc_path`, `get_icons_path` feed docs/icons.
- `env.module_add_dependencies()` and `env.module_check_dependencies()` enforce required/optional module deps.
- Enabled modules produce `modules/modules_enabled.gen.h`, `modules/register_module_types.gen.cpp`, and per-module `libmodule_<name>` libraries.
- `MODULE_<MODULE>_ENABLED` defines come from enabled module list.