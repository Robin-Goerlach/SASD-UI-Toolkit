# Roadmap

This roadmap describes technical direction, not release-date commitments. The project prioritizes small, testable milestones and an API that can grow without locking in unnecessary complexity too early.

## M0 – Architecture and repository foundation

**Goal:** establish shared language, project scope and architectural boundaries.

- [x] Repository and MIT license
- [x] Document vision and scope
- [x] Define backend/peer strategy
- [x] Treat terminal as a first-class backend
- [x] Reserve Model/View as the long-term basis for data-heavy widgets
- [x] Create German/English documentation structure
- [x] Introduce Architecture Decision Records (ADR) for durable rationale
- [x] Define public naming without vendor-prefixed class names
- [x] Keep the visual designer/RAD environment outside the core toolkit

## M1 – Core skeleton and headless validation

**Status: completed on 2026-09-18.** The planned core building blocks and exit criterion are satisfied; additional focus, layout and presentation contracts are also covered headlessly.

**Goal:** make the platform-neutral core executable and testable without a real display backend.

Planned:

- CMake project
- `sasd::ui` namespace
- `Application`
- `Component`
- `Widget`
- `Container`
- geometry and size-constraint types
- foundational event model
- event queue / dispatcher
- backend interface
- capability model
- deterministic headless/mock backend
- reusable backend contract tests
- unit-test foundation
- CI for Windows, Linux and macOS

The mock backend should validate component trees, ownership, events, focus, layout and backend contracts without requiring a terminal, display server or native window system.

**Exit criterion:** the core builds with GCC, Clang and MSVC without requiring a concrete GUI framework, and representative core behavior passes against the headless/mock backend.

## M2 – Terminal Preview / v0.1.0

**Status: completed (2026-09-28).** The terminal-preview slice is implemented and the exit criterion has been manually validated in a Linux/xterm-like environment (WSL2, `TERM=xterm-256color`) and in Windows Terminal. Automated core/terminal tests, POSIX PTY/Windows ConPTY smoke tests, sanitizers, and cross-platform CI remain green. Richer Unicode, keyboard, and pointer functionality deliberately remains follow-up work and does not block v0.1.0.

**Goal:** first genuinely usable user-visible vertical slice.

Planned:

- terminal backend
- `Window`/screen concept
- `Label`
- `Button`
- `TextField`
- `VBox`
- `HBox`
- focus navigation
- keyboard and text input
- resize handling
- simple styles
- example applications
- backend contract tests

**Exit criterion:** a small interactive application runs in at least a Linux/xterm-like environment and a modern Windows console/terminal with substantially identical application code.

The binding manual completion procedure is documented in the [M2 terminal smoke test](M2_TERMINAL_SMOKE_TEST.md).

## M3 – Rendered Desktop Preview / v0.2.0

**Status: in progress (since 2026-09-28).** v0.1.0/M2 is published. The separate `SASD::UI::Rendered` target, deterministic `rendered::DisplayList`, `RenderedPresentationSink`, `RenderedMeasurementContext`, `RenderDevice`, and `DisplayListExecutor` are implemented and tested cross-platform. Window, Label, Button and metric-backed TextField translate into ordered, clipped commands. A first concrete, experimental build-tree-only `SASD::UI::Rendered::SDL3` adapter now executes that command stream through a real headless SDL3 software renderer and SDL_ttf, with actual UTF-8 rasterization, clipping, font metrics and shaped scalar-boundary queries covered by dedicated CI. Generic Core/Rendered APIs still contain no SDL types and normal builds remain SDL-independent. A window-backed SDL3 host/device is now implemented with transactional lifecycle, complete-frame presentation, resize/close/key/committed-text translation, logical/pixel/display-scale separation and an offscreen CI vertical slice. Backend-neutral pointer events, reverse-order visual hit testing, handler-owned capture and Button press/release semantics are now implemented, including Terminal/Rendered pressed presentation and SDL3 logical-coordinate translation. Rendered TextField click-to-caret now reuses the same shaping metrics and horizontal viewport as presentation. A first geometry-only `RenderedThemeMetrics` policy now keeps border, pressed-offset and caret-width measurement/presentation aligned without introducing a full theme system. PointerRouter now also maintains lifetime-safe root-to-target geometric hover independently from capture, with a first Rendered Button hover cue. SDL3 window enter/leave is represented separately as top-level pointer-surface lifecycle so native window leave retires hover/capture without synthetic coordinates. Visible Windows 11 / MSVC / SDL3 validation is now passed: click-to-caret, pointer capture, keyboard activation, resize semantics and the immediate replay/present resize retest all behaved as expected. Visible Debian 12 / WSL2 / WSLg validation has also passed. The remaining M3 platform gate is the visible macOS/Cocoa validation.

**Goal:** demonstrate the same public API graphically across desktop platforms.

Planned:

- optional SDL3 backend or similarly small rendering/input layer
- desktop window
- text/font metrics
- mouse/pointer input
- focus
- rendering for the existing base widgets
- high-DPI foundations
- theme metrics

**Exit criterion:** the M2 sample application also runs in a graphical desktop window on Windows, Linux and macOS without exposing SDL types to application code.

## M4 – Layout, commands and form controls / v0.3.x

**Status: started 2026-09-30.** The outstanding visible macOS/Cocoa M3 validation remains an external platform check and does not block backend-neutral M4 development. The semantic two-state `CheckBox` and the `RadioButton`/`RadioGroup` slice are implemented across Core, Terminal and Rendered presentation and are integrated into the sample applications; dedicated SDL3 integration paths exercise their native-pointer-to-frame behavior. The identical primary-pointer press/capture/release mechanics proven across Button, CheckBox and RadioButton have been factored into a private Core helper without introducing a public control base class. `RadioGroupNavigation` provides explicit cyclic Left/Up and Right/Down navigation in stable group order, skips effectively unreachable members, and never crosses a top-level visual root.

The first M4 layout family is now implemented and stabilized. `GridLayout` provides a fixed positive column count, dense row-major placement of visible children, intrinsic column/row maxima, independent row/column spacing and deterministic clipping under shortage. Track occupancy is structural: an occupied zero-width column still exists and keeps the spacing boundary to the preceding occupied column. `FormLayout` is a separate implementation with stable label/field pairing by visual adoption order, a shared intrinsic label column, an expanding field column, label-only final rows, hidden-row handling and deterministic clipping. Visibility never repacks its structural pairs; explicit `release()`/re-adoption is a structural change and therefore follows the resulting child order. `StackLayout` overlays every visible child into the same client rectangle; desired size is the component-wise maximum, adoption order defines paint order, and later children are therefore topmost. Terminal and Rendered presentation treat Grid/Form/Stack as explicit structural containers, while unknown concrete `Container` subclasses remain deferred so future composite controls cannot silently lose presentation state. Visibility changes conservatively request subtree replay, and forced replay prunes hidden widgets so an uncovered lower stack layer is restored correctly.

The current layout contracts are intentionally small. `GridLayout` does not yet provide spans, weighted tracks, per-cell alignment, margins/padding or extra-space distribution. `FormLayout` does not yet provide per-row metadata, independent row objects or alignment policy beyond the shared two-column contract. `StackLayout` does not provide active-page state, z-index, offsets, margins or window-manager semantics; visibility is currently the mechanism for selecting layered content. Those features remain later extensions and should only be added when real consumers justify the additional public API.

Planned:

- [x] `GridLayout`
- [x] `FormLayout`
- [x] `StackLayout`
- [x] `CheckBox`
- [x] `RadioButton`
- [ ] `ComboBox`
- [ ] commands/actions
- [ ] semantic menu model
- [ ] shortcuts
- [ ] clipboard foundation
- [ ] binding/validation foundations where supported by real use cases

## M5 – Model/View and data-heavy widgets / v0.4.x

Planned:

- `ListModel`
- `TableModel`
- `TreeModel`
- `ListView`
- `TableView`
- `TreeView`
- selection models
- delegate/cell-renderer concept
- virtualization for large data sets

This milestone is particularly important for scientific, statistical and engineering applications.

## M6 – Native desktop peers

Native backends intentionally begin **after** the core contract has gained practical stability.

Planned targets:

- Windows/Win32
- Linux/GTK
- macOS/AppKit

Order and exact scope will be chosen after lessons from M3/M4.

## M7 – Desktop integration

Longer-term areas:

- native menus
- file dialogs
- drag and drop
- advanced clipboard
- system tray
- notifications
- accessibility bridges
- IME
- printing
- theme/appearance integration

## M8 – Developer productivity foundations

Only after the component model is sufficiently stable:

- component metadata
- serialization of UI descriptions
- resource system
- designer-friendly properties
- design-time validation metadata
- IDE/tooling integration points

A full visual designer/RAD environment is intentionally a **separate sister project**, not part of the toolkit core.

## Deliberately later

Do not prioritize too early:

- a complete custom 2D/3D graphics engine
- web renderer
- Android/iOS
- animation framework
- full rich-text stack
- browser engine
- maximizing widget count merely for a feature checklist
- stable binary ABI before the architecture has survived multiple real backends

## Release principle

A release should provide a **usable and tested vertical slice**. Six widgets that work correctly across two backends are more valuable than fifty partially implemented components.

Pre-1.0 releases may contain source-breaking changes when necessary to correct the architecture. Such changes must be documented clearly.