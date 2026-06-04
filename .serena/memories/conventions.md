# Conventions

C++ style:
- `.clang-format` based on LLVM, indent width 4, column limit 0, include blocks preserved, case-sensitive include sort, pointer alignment right, constructor initializers after colon/next line.
- Headers use Godot banner then `#pragma once`; `misc/scripts/header_guards.py` converts legacy guards to `#pragma once` after banner.
- Format file hygiene: `misc/scripts/file_format.py` strips trailing whitespace, normalizes newline, UTF-8, CRLF only for `.csproj/.sln/.bat` and `misc/msvs`, BOM only for `.csproj/.sln`.
- Godot naming visible in code: `p_` params, `r_` out params common, `m_` macro params, snake_case methods/properties exposed to scripts, PascalCase types, UPPER_CASE enum constants/macros.
- Error/guard style uses Godot macros (`ERR_FAIL_*`, `ERR_FAIL_COND*`, etc.) rather than exceptions for engine logic; exceptions default disabled in SCons.

API conventions:
- Script-exposed API must be registered through `ClassDB`/`GDCLASS` patterns and documented in `doc/classes/*.xml`.
- Resource classes often define base extension/saved type. Do not bypass `Resource` cache/path/local-scene behavior when editing resources.
- Prefer engine containers/types (`String`, `StringName`, `Vector`, `LocalVector`, `HashMap`, `Ref`, `RID`, `Variant`) over STL in core engine surfaces unless local code already uses STL.

Python/build scripts:
- `pyproject.toml`: ruff target py38, line length 120, SConstruct/SCsub special import-star ignores; mypy excludes thirdparty and targets Python 3.8.