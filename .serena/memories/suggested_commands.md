# Suggested Commands

Build:
- `scons platform=linuxbsd target=editor dev_mode=yes` -> local Linux editor dev build with tests/werror/strict checks defaults.
- `scons platform=linuxbsd target=editor tests=yes` -> build tests without all dev-mode defaults.
- `scons platform=linuxbsd target=editor compiledb=yes` -> generate `compile_commands.json` for clang tooling.
- `scons platform=linuxbsd target=editor ninja=yes` -> generate/run ninja backend if SCons >= 4.2.

Run tests after building with `tests=yes`:
- `bin/godot.linuxbsd.editor.dev.x86_64 --test` -> likely dev editor test binary name on Linux x86_64; verify exact suffix in `bin/` if build options differ.
- `bin/godot.linuxbsd.editor.dev.x86_64 --test --test-case="*StyleBox*"` -> doctest filter example.
- `bin/godot.linuxbsd.editor.dev.x86_64 --test gdscript` or other registered custom command if available.

Format/lint targeted files:
- `clang-format -i <changed .h/.cpp/.mm/.m/.java files>` -> C++/ObjC/Java formatting per `.clang-format`.
- `python3 misc/scripts/file_format.py <changed files>` -> newline/trailing whitespace/BOM normalization.
- `python3 misc/scripts/header_guards.py <changed headers>` -> enforce Godot `#pragma once` placement.
- `python3 misc/scripts/validate_xml.py doc/classes/<Class>.xml` -> validate touched class docs.
- `clang-tidy <file> -- -I. ...` needs compile command context; prefer generated `compile_commands.json` and editor/clangd diagnostics for practical use.

Useful discovery:
- `rg --files <dirs>` for file map.
- `rg -n "GDCLASS\(|ClassDB::bind_method|ADD_PROPERTY|VARIANT_ENUM_CAST" <dirs>` for API binding search.
- `rg -n "class StyleBox|GDCLASS\(StyleBox" scene/resources/style_box.h doc/classes/StyleBox.xml` for class/base verification.