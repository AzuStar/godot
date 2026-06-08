# Testing

- In this repo run tests through the built binary with writable XDG paths, e.g. `XDG_CACHE_HOME=/tmp XDG_CONFIG_HOME=/tmp XDG_DATA_HOME=/tmp bin/godot.linuxbsd.editor.dev.x86_64 --headless --test`.
- Sandbox/host may break network/IPC/HOME-dependent tests. Verified passing filter: `--test-case-exclude='*[IP]*,*[Logger]*,*[TCPServer]*,*[UDPServer]*,*[UDSServer]*,*[StreamPeerTCP]*,*[HTTPClient]*'` -> 1250 passed, 0 failed.

- C++ tests are doctest-based. Build with `tests=yes`; `dev_mode=yes` also enables tests unless overridden.
- Test library source: `tests/SCsub`; main runner: `tests/test_main.cpp`; helpers/macros: `tests/test_macros.h`, `tests/test_utils.*`.
- Runtime entry is engine binary with `--test`; `test_main()` strips `--test` before passing remaining args to doctest.
- Custom test commands use `REGISTER_TEST_COMMAND("name", function)` and run via `godot --test name`; if matched, doctest suite is skipped.
- Doctest filtering uses regular doctest CLI args after `--test`, e.g. `--test --test-case="..."`.
- Tests set `DisplayServerMock`, initialize `WorkerThreadPool`, clear `TestUtils::get_temp_path("")`, and use Godot-style test macros (`TEST_COND`, `TEST_FAIL_COND`, etc.).
- Module tests: when `tests=yes`, `modules/SCsub` scans enabled module `tests/*.h`, generates `modules/modules_tests.gen.h`, and `tests/test_main.cpp` includes it.
- Windows doctest thread-local is disabled; if `disable_exceptions=yes`, tests define `DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS`.