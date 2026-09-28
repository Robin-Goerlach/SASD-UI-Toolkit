# Changelog

All notable changes to SASD UI Toolkit are documented in this file.

The project is still pre-1.0. Source-breaking corrections may occur between feature releases when
they improve the architecture before compatibility commitments become expensive.

## [0.2.0] - Unreleased

### Added

- M3 Rendered Desktop Preview development started.
- Separate `SASD::UI::Rendered` target with a deterministic, SDL-independent `rendered::DisplayList`.
- Initial rendered command vocabulary for filled rectangles, stroked rectangles and owned UTF-8 text.
- Cross-platform tests for command ordering, malformed geometry, ownership and package/export behavior.
- Optional per-text-command clipping bounds for safe widget-local rendering without a premature graphics-state stack.
- `RenderedPresentationSink` for Window refreshes, Label and Button presentation, including incremental repaint and conservative subtree rebuild behavior.
- `RenderedMeasurementContext` with logical line-height and deferrable Unicode-scalar boundary advances for rendered font/shaping metrics.
- Metric-aware rendered `TextField` presentation with full-run clipping, scalar-boundary horizontal viewport and insertion caret, while unsupported caret mappings remain explicitly deferred.

## [0.1.0] - 2026-09-28

### Added

- C++20 platform-neutral Core with `Application`, `Component`, `Widget`, `Container`, geometry,
  event routing, focus management, two-phase layout, measurement contexts and presentation
  synchronization.
- Deterministic headless/mock backend and reusable backend-contract tests.
- First-class Terminal backend with off-screen `ScreenBuffer`, ANSI/VT frame encoding, native
  POSIX and Windows terminal sessions, non-blocking input decoding and resize handling.
- M2 widgets and layouts: `Window`, `Label`, `Button`, single-line UTF-8 `TextField`,
  `VBox` and `HBox`.
- Tab/Shift+Tab focus traversal, F1-F12 key identities, Enter/Space button activation and
  backend-neutral text styling.
- Versioned terminal Unicode cell-width handling with conservative deferral for unsupported
  combining/ZWJ sequences.
- Runnable `sasd_ui_terminal_demo` used by the Linux/xterm-like and Windows Terminal manual
  release gate.
- Native process smoke tests through POSIX PTY and Windows ConPTY, plus GCC/Clang/AppleClang/MSVC
  and ASan/UBSan CI coverage.
- Installable CMake package exporting the same consumer targets as the build tree:
  `SASD::UI` and `SASD::UI::Terminal`.
- Cross-platform installed-package consumer smoke test in CI.

### Validation

- Linux/xterm-like M2 smoke test: PASS.
- Windows Terminal M2 smoke test: PASS, including `Robin AΩ界` Unicode input/editing.
- Terminal state/code-page restoration validated on all required exit paths.

### Not yet included

- Mouse/pointer input.
- Complete grapheme-cluster/emoji/ZWJ cell editing.
- Bracketed paste and extended terminal keyboard protocols.
- RGB/true-color and background/theme cascade.
- Rendered desktop backend and native desktop peers.

