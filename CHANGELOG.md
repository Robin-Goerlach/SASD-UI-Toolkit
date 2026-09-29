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
- Geometry-only `RenderedThemeMetrics` shared by rendered control measurement, Button/TextField chrome, TextField viewport/caret layout and click-to-caret mapping.
- Metric-aware rendered `TextField` presentation with full-run clipping, scalar-boundary horizontal viewport and insertion caret, while unsupported caret mappings remain explicitly deferred.
- `RenderDevice` and `DisplayListExecutor` execution boundary, centralizing deterministic command replay for later SDL3/native adapters without exposing their types.
- `FillRole` on rendered rectangle fills so `Color::default_color` preserves foreground-vs-background intent (notably TextField caret versus widget erasure).
- Experimental build-tree-only `SASD::UI::Rendered::SDL3` adapter using a real SDL3 software renderer and SDL_ttf for UTF-8 rendering, clipping, real font metrics and shaped caret-boundary queries.
- Dedicated headless SDL3 adapter CI with pinned SDL/SDL_ttf releases and externally supplied test font; normal toolkit builds remain SDL-independent.
- Reproducible visible Linux M3 smoke helper with pinned SDL fetch, font discovery, X11/Wayland build validation, session diagnostics and explicit human PASS gate.
- SDL_ttf FetchContent configuration no longer FORCE-overwrites a parent project's generic `BUILD_SHARED_LIBS` cache policy; CI guards the embedding invariant.
- Fetched static SDL3/SDL_ttf dependencies are built position-independent so a parent `BUILD_SHARED_LIBS=ON` policy can produce shared SASD adapter libraries without ELF relocation failures.
- SDL3 command-local text clipping now restores pre-existing renderer clip state instead of assuming an unclipped outer renderer.
- Window-backed experimental SDL3 backend with transactional lifecycle, complete-frame presentation, resize/close/key/text event translation and logical/pixel/display-scale separation.
- High-DPI regression coverage keeps pixel-size/display-scale changes presentation-only: they request replay without manufacturing logical resize events or metric revisions.
- `PresentationCoordinator::replay()` for deterministic full-tree reconstruction after native presentation-surface loss/expose without mutating semantic dirty state.
- Runnable `sasd_ui_sdl3_demo` plus end-to-end offscreen SDL window tests.
- Backend-neutral `PointerEvent`, visual-tree `HitTest` and lifetime-safe `PointerRouter` with capture assigned to the Widget that handles the press.
- Lifetime-safe root-to-target geometric hover state independent from capture, plus a geometry-neutral Rendered Button hover overlay.
- Backend-neutral `PointerSurfaceEvent` plus SDL3 window enter/leave translation; surface leave clears hover and conservatively retires semantic capture.
- Semantic `Button::isPressed()` state with capture-loss cleanup, release-inside activation / release-outside cancellation, and Terminal/Rendered pressed presentation.
- SDL3 pointer coordinate conversion and private deterministic semantic translation seam; the SDL3 backend now advertises pointer input capability.
- Shared private Rendered TextField viewport geometry plus `RenderedTextFieldHitTest` for metric-correct pointer click-to-caret mapping without leaking font/pixel state into Core.
- SDL3 integration coverage from native mouse-button representation through semantic cursor placement to the final rendered TextField caret command.

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

