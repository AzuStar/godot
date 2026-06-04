# Task Completion

Minimum after C++ edits:
- Format touched C++/headers with `clang-format -i <files>`.
- Run `python3 misc/scripts/file_format.py <files>` on touched text files.
- Run `python3 misc/scripts/header_guards.py <headers>` for touched/new headers.
- Run focused build: `scons platform=linuxbsd target=editor tests=yes` or narrower target if user/project context gives one.
- Run focused test binary command after build: `bin/godot.linuxbsd.editor*.x86_64 --test` with doctest filter for touched area when possible.

When API exposed to scripts changes:
- Update matching `doc/classes/<Class>.xml`.
- Validate XML with `python3 misc/scripts/validate_xml.py doc/classes/<Class>.xml`.
- Check generated docs/API expectations if adding/removing methods/properties/signals/enums.

When module changes:
- Confirm module has/keeps `config.py`, `SCsub`, `register_types.h`.
- If tests added in `modules/<name>/tests/*.h`, build with `tests=yes` so `modules_tests.gen.h` includes them.
- Verify module deps through `env.module_add_dependencies`/`env.module_check_dependencies` if new dependencies are introduced.

If only answering symbol/query with no edits:
- No formatter/build required. Prefer Serena symbol lookup + local source/docs confirmation.