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

**Status: in progress.** The separate `SASD::UI::Terminal` target, off-screen cell surface, `Window`/`Label`, Unicode cell metrics, backend-neutral MeasurementContext, `VBox`/`HBox`, `Button`, and a single-line `TextField` with UTF-8 editing, cursor, horizontal viewport and separate terminal caret are implemented. Tab/Shift+Tab focus traversal is also implemented. ANSI/VT frame output and real terminal I/O/input translation follow next.

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

## M3 – Rendered Desktop Preview / v0.2.0

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

Planned:

- `GridLayout`
- `FormLayout`
- `StackLayout`
- `CheckBox`
- `RadioButton`
- `ComboBox`
- commands/actions
- semantic menu model
- shortcuts
- clipboard foundation
- binding/validation foundations where supported by real use cases

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