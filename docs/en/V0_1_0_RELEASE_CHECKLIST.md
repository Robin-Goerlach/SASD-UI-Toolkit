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

- [x] Final release commit `dc34ebd4bf6db4a0f9777fc81320f47dae72b36b`; complete CI for that exact SHA is green.
- [x] Produced and tested Release builds from fresh CI checkouts on Linux, macOS and Windows.
- [x] Automatically verified installed public headers, Core/Terminal libraries, CMake config, license/README/changelog.
- [x] Finalized bilingual release notes from `CHANGELOG.md`.
- [x] Release CI verifies repository hygiene; no generated build/linker artifacts are tracked.
- [x] Explicit publication approval received; CI-gated publisher prepared.
- [x] Annotated tag `v0.1.0` points exactly at `dc34ebd4bf6db4a0f9777fc81320f47dae72b36b`.
- [x] GitHub Release `SASD UI Toolkit v0.1.0` published.

## Release boundary

v0.1.0 is a **pre-1.0 release**. It records the first usable terminal vertical slice without freezing
long-term source compatibility. Larger API corrections may still occur before 1.0 and must remain
documented and reviewable.
