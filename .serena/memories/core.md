# Core

- Godot Engine fork, C++ engine source. Version metadata in `version.py`: 4.6.3 stable, docs branch 4.6.
- Read `mem:architecture/source_map` for major dirs, build order, generated artifacts, and where to start for subsystem work.
- Read `mem:build/scons` for SCons options, module detection/registration, generated headers, and compile-db/ninja behavior.
- Read `mem:api/classdb` for native class inheritance/binding/docs conventions (`Object`, `RefCounted`, `Resource`, `Node`, `ClassDB`, `GDCLASS`).
- Read `mem:testing` for doctest runner behavior, test registration, module test inclusion, and temp/mock setup.
- Read `mem:tech_stack` for toolchain/language/version pins.
- Read `mem:conventions` for local code style and exposed API documentation obligations.
- Read `mem:suggested_commands` for build/test/format/lint commands that are worth running locally.
- Read `mem:task_completion` for minimum done checks after edits.

Durable invariants:
- Root `SConstruct` owns platform/module discovery, global options, and library link order.
- Native exposed classes live mostly under `core`, `scene`, `servers`, `editor`, `modules`; script-visible API comes from native bindings plus `doc/classes/*.xml`.
- Generated files are common (`*.gen.h`, `*.gen.cpp`); do not edit generated outputs unless generation source changed or task explicitly targets generated artifacts.
- Third-party code mostly under `thirdparty`; SCons clones env and often disables warnings for bundled third-party sources.