# v0.1.0 Release Checklist

Status: 2026-09-28

This checklist separates a **technically releasable state** from the later publication step. A Git tag
or GitHub release is created only after explicit approval.

## Already satisfied

- [x] M2 Terminal Preview functionally complete.
- [x] Linux/xterm-like manual smoke test passed.
- [x] Windows Terminal smoke test passed, including `Robin AΩ界`.
- [x] TTY/code-page restoration passed for F10, Escape and Exit-button paths.
- [x] Linux GCC, Linux Clang, macOS AppleClang and Windows MSVC CI coverage.
- [x] Linux Clang ASan + UBSan CI coverage.
- [x] CMake project version is `0.1.0`.
- [x] Installable CMake package available.
- [x] External consumer can use `find_package(SASDUIToolkit 0.1 CONFIG REQUIRED)`.
- [x] Installed targets match build-tree names: `SASD::UI` and `SASD::UI::Terminal`.
- [x] v0.1.0 changelog created.

## Still verify before publication

- [ ] Record the final `main` HEAD and confirm the complete CI run for that exact HEAD is green.
- [ ] Produce a Release build from a fresh checkout locally or in CI.
- [ ] Spot-check installed contents: headers, libraries, CMake config, license/README/changelog.
- [ ] Finalize release notes from `CHANGELOG.md`.
- [ ] Verify no accidental build artifacts or local files are tracked.
- [ ] Only after explicit approval: create tag `v0.1.0`.
- [ ] Only after explicit approval: publish the GitHub Release.

## Release boundary

v0.1.0 is a **pre-1.0 release**. It records the first usable terminal vertical slice without freezing
long-term source compatibility. Larger API corrections may still occur before 1.0 and must remain
documented and reviewable.
