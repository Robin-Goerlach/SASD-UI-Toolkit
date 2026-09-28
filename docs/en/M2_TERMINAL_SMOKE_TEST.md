# M2 Terminal Smoke Test

Status: 2026-09-28

This document defines the **manual exit validation for M2 / v0.1.0**.

Automated CI already covers core/terminal unit tests, sanitizers, Linux/macOS PTY and Windows ConPTY.
The remaining intentionally human validation step is interaction inside real terminal emulators.

## Release gate

M2 is manually validated only after the demo passes without critical failure in both:

- Linux in an xterm/VT-compatible terminal environment;
- Windows in Windows Terminal or another modern Windows console.

Both environments must run the same `examples/terminal_form_demo.cpp`.

Critical failures include raw-mode/cursor restoration problems, broken shell echo/line editing, frozen
input, persistent resize corruption, broken focus traversal, UTF-8 corruption, or exit paths that leave
terminal state changed.

## 1. Preferred: use the smoke runners

Two small helper scripts prepare build/test, environment information and restoration checks while
leaving visual/interactive judgment intentionally human.

Linux/macOS:

```bash
./tools/m2_smoke_posix.sh
```

Windows PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\m2_smoke_windows.ps1
```

By default the runners launch the demo **three times**: first for the complete interaction pass ending
with F10, then once each for Escape and the Exit button. Restoration is therefore checked separately
after all three required exit paths. Add `--single-run` or `-SingleRun` for a quick diagnostic run; a
single run alone is not sufficient to close the M2 release gate.

After an already successful build, use `--skip-build` or `-SkipBuild`. The helpers defensively restore
the original TTY/code-page state while still reporting any mismatch as a failure.

For a release-gate run both helpers also require a **clean Git worktree**, making the result
unambiguously attributable to the recorded commit. The generated platform-specific `build-smoke-posix/` and `build-smoke-windows/` directories are
therefore ignored repository-wide.

After the three normal runs the human visual/interaction check must be explicitly confirmed by typing
`PASS`. Only then does the helper report `M2 environment result: PASS`. The `--single-run` /
`-SingleRun` mode remains diagnostic and does not confirm the release gate.

## Troubleshooting local preparation

If Linux reports `/usr/bin/env: ‘bash\\r’: No such file or directory` when starting the shell helper,
the local file has CRLF line endings. The repository therefore enforces LF for `*.sh` through
`.gitattributes`. In an older checkout, preserve or discard local changes as appropriate and check
out the current `main` again.

On Windows the smoke runner first looks for CMake/CTest on `PATH`. If they are not available there,
it uses Visual Studio's `vswhere.exe` to locate the CMake tools installed with Visual Studio. Only
when neither source provides the tools does the runner stop with an installation hint.

Windows PowerShell 5.1 interprets UTF-8 script files without a BOM through the legacy ANSI code page.
The Windows runner therefore deliberately keeps its source ASCII-only and constructs the
`Robin AΩ界` test sample from Unicode code points at runtime. This makes the smoke test validate
terminal presentation rather than accidental source-file decoding.

WSL/Linux and native Windows deliberately use **different CMake build directories**:
`build-smoke-posix/` and `build-smoke-windows/`. A CMake cache contains absolute source/build
paths plus generator/toolchain state, so the same checkout must not reuse one smoke build tree from
both WSL and Windows.

## 2. Verify the automated baseline manually

```bash
cmake -S . -B build-smoke-posix \
  -DCMAKE_BUILD_TYPE=Debug \
  -DSASD_UI_BUILD_TESTS=ON \
  -DSASD_UI_BUILD_EXAMPLES=ON \
  -DSASD_UI_WARNINGS_AS_ERRORS=ON

cmake --build build-smoke-posix --parallel
ctest --test-dir build-smoke-posix --output-on-failure
```

Windows / Visual Studio:

```powershell
cmake -S . -B build-smoke-windows ^
  -G "Visual Studio 17 2022" ^
  -A x64 ^
  -DSASD_UI_BUILD_TESTS=ON ^
  -DSASD_UI_BUILD_EXAMPLES=ON ^
  -DSASD_UI_WARNINGS_AS_ERRORS=ON

cmake --build build-smoke-windows --config Debug --parallel
ctest --test-dir build-smoke-windows -C Debug --output-on-failure
```

## 3. Record the environment

| Field | Value |
|---|---|
| Commit | |
| Operating system | |
| Terminal emulator | |
| Shell | |
| Initial terminal size | |
| Keyboard layout | |
| Result | PASS / FAIL |

## 4. Linux / xterm-like environment

Optional POSIX TTY state comparison:

```bash
before="$(stty -g)"
./build-smoke-posix/examples/sasd_ui_terminal_demo
after="$(stty -g)"

if [ "$before" = "$after" ]; then
  echo "TTY restoration: PASS"
else
  echo "TTY restoration: FAIL"
fi
```

Validate clean startup, style/color presentation, `Robin AΩ界` input, editing/navigation keys,
Tab/Shift+Tab, Enter/Space button activation, F1 Help, F10 exit, shrink/enlarge resize, Escape exit, Exit-button exit and
normal cursor/echo/shell editing after termination. `stty -g` should match before and after.

Terminal themes may map named colors differently; leaking SGR state or raw escape text is not
acceptable.

## 5. Windows Terminal

```powershell
.\build-smoke-windows\examples\Debug\sasd_ui_terminal_demo.exe
```

Optional visible code-page comparison:

```powershell
$before = (chcp)
.\build-smoke-windows\examples\Debug\sasd_ui_terminal_demo.exe
$after = (chcp)

"Before: $before"
"After:  $after"
```

Repeat the Linux functional validation: clean startup, styles, `Robin AΩ界`, editing/navigation,
Tab/Shift+Tab, Enter/Space activation, F1 Help, F10 exit, resize, Escape, Exit button, and normal shell behavior after
termination. The console code page must be restored to its pre-run state.

## 6. Deliberately outside the M2 release gate

Not M2 failures when the documented conservative behavior is preserved:

- complete combining/ZWJ/emoji grapheme-cell editing;
- mouse/pointer interaction;
- F13+ and extended keyboard protocols;
- Kitty Keyboard Protocol / complete CSI-u;
- bracketed paste;
- arbitrary RGB/true-color styling;
- background styling and theme cascade.

## 7. Completion record

| Environment | Commit | Result | Notes |
|---|---|---|---|
| Linux / xterm-like | `25c3522` | PASS | WSL2 Linux 5.15.167.4, `TERM=xterm-256color`; build/CTest, all three exit paths, TTY restoration and the human visual/interaction check passed. |
| Windows Terminal | | PASS | |

Only after both rows pass with no critical failures should the roadmap mark the M2 exit criterion as
satisfied and v0.1.0 release preparation begin.
