# SASD UI Toolkit

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
![Project status](https://img.shields.io/badge/status-M3%20rendered%20desktop%20preview-orange)
![C++ target](https://img.shields.io/badge/C%2B%2B-20-blue)

**Open-source C++ UI toolkit for building portable desktop and terminal applications across Windows, Linux and macOS.**

> **Current status:** v0.1.0 (M2 Terminal Preview) is published. Development has moved to M3 / v0.2.0, introducing a rendered desktop presentation path while preserving the backend-neutral Core and Terminal backend.

SASD UI Toolkit aims to provide a small, understandable and extensible component API that can target very different presentation environments without forcing normal application code to depend on a specific native GUI toolkit.

The project is inspired by proven ideas from **Delphi VCL, Java AWT/Swing, Qt, wxWidgets, FLTK, FTXUI and Turbo Vision**, while deliberately remaining an independent modern C++ design.

---

## Why another UI toolkit?

The project explores a specific combination that is not the primary design goal of most existing frameworks:

- one semantic C++ component API;
- Windows, Linux and macOS as desktop targets;
- terminal/ANSI/VT environments as a **first-class target**, not an afterthought;
- native controls where they provide a platform advantage;
- rendered widgets where native controls are unavailable or inappropriate;
- a small platform-neutral core;
- permissive MIT licensing with no SASD UI Toolkit license fees;
- architecture suitable for classic desktop, administration, engineering and data-oriented applications.

The goal is **not** to clone VCL, Swing, Qt or wxWidgets. The goal is to learn from their strongest ideas and combine them into a coherent toolkit for modern C++.

## Current foundation

The repository now contains the first working implementation slice:

- CMake-based C++20 library target `SASD::UI`;
- `Application`, `Component`, `Widget` and `Container` foundations;
- explicit distinction between component ownership and visual parenting;
- backend-neutral geometry and sizing (`Point`, `Size`, `Rect`, `SizeConstraints`, `MeasureConstraints`);
- backend capabilities model;
- key, text, focus, pointer, resize and quit event types;
- thread-safe FIFO `EventQueue`;
- target-to-parent event routing with explicit handled/ignored semantics;
- lifetime-safe logical keyboard `FocusManager` plus deterministic visual-tree `FocusTraversal` for Tab/Shift+Tab;
- backend-neutral `HitTest` and lifetime-safe `PointerRouter` with handler-owned pointer capture;
- two-phase `measure()` / `arrange()` layout foundation with cached desired sizes and invalidation propagation;
- backend-neutral `MeasurementContext` so content widgets can obtain terminal/font/native metrics without backend coupling;
- first automatic layout containers `VBox` and `HBox`, with context propagation, spacing, visibility collapse and deterministic constrained arrangement;
- separate backend-neutral visual-update invalidation so redraw/synchronization does not force re-measurement;
- conservative presentation-subtree refresh for move/resize/remove damage before optimized dirty regions exist;
- deterministic `PresentationCoordinator` / `PresentationSink` bridge for pending visual updates;
- separately linkable `SASD::UI::Terminal` M2 target with a tested off-screen terminal `ScreenBuffer`;
- separately linkable `SASD::UI::Rendered` M3 target with a deterministic backend-neutral `DisplayList` and `RenderedPresentationSink` for rendered drawing commands;
- `RenderedMeasurementContext` for coherent rendered font line-height/scalar-boundary metrics shared by layout and TextField viewport/caret presentation;
- `RenderDevice` plus `DisplayListExecutor` as the SDL/native-free execution boundary that replays immutable drawing commands into a concrete renderer;
- experimental build-tree-only `SASD::UI::Rendered::SDL3` adapter with headless software rendering plus a real window-backed backend using SDL3 + SDL_ttf;
- explicit `PresentationCoordinator::replay()` for rebuilding an exposed/lost presentation surface without abusing semantic Widget invalidation;
- semantic M2 widgets: `Window`, UTF-8 `Label`, interactive `Button` and single-line editable `TextField`;
- headless `TerminalPresentationSink` that renders `Window`, `Label`, `Button` and `TextField` through `PresentationCoordinator` into terminal cells;
- versioned terminal `TextMetrics` with UTF-8 decoding, narrow/wide/ambiguous cell widths and explicit wide-cell occupancy;
- `TerminalMeasurementContext`, giving `Label`, `Button` and `TextField` real terminal-cell desired sizes while keeping core widgets terminal-agnostic;
- keyboard- and pointer-activatable `Button` with semantic pressed state, capture-safe press/release behavior and Terminal/Rendered pressed presentation;
- `TextField` editing through `TextInputEvent` plus scalar cursor/navigation/delete semantics, shared UTF-8 decoding, terminal horizontal scrolling and separate hardware-caret position;
- rendered TextField click-to-caret mapping that reuses the exact shaping metrics and horizontal viewport used by presentation;
- deterministic `AnsiFrameEncoder` converting `ScreenBuffer` plus optional caret into tested full-frame UTF-8/ANSI-VT bytes without OS I/O;
- RAII `TerminalSession` / `TerminalDevice` boundary with deterministic mock tests and native POSIX/Windows adapters for TTY/console sizing, raw/VT session state, alternate screen and byte transport;
- non-blocking terminal input polling plus incremental `AnsiInputDecoder` translating split UTF-8/CSI/SS3 streams into existing `KeyEvent` / `TextInputEvent` semantics, including portable F1–F12 function-key identities and xterm modifiers;
- `TerminalEventPump` and concrete `TerminalBackend : Backend` integrating resize, input timing and terminal session lifecycle with the normal `Application` event path;
- buildable `sasd_ui_terminal_demo` exercising real terminal input/output, resize, TextField editing, Tab/Shift+Tab focus, Buttons and RAII terminal restoration;
- minimal backend-neutral `Color` / `TextStyle` support for Label/Button/TextField with 16 portable foreground colors plus bold/dim/underline/inverse, carried through terminal cells into ANSI SGR;
- deterministic `MockBackend` for headless contract testing;
- dependency-free unit-test harness integrated with CTest;
- warnings-as-errors support;
- AddressSanitizer/UndefinedBehaviorSanitizer support;
- native process-level terminal smoke tests: real POSIX PTY coverage on Linux/macOS and real Windows ConPTY coverage for session mode/code-page changes, size discovery, raw VT input, frame output and RAII restoration;
- GitHub Actions matrix for GCC, Clang, MSVC and AppleClang.

The terminal backend is the completed v0.1.0 reference implementation for a visible backend. M3 now adds a rendered desktop path while keeping the same semantic widgets, layout, focus, events and presentation coordination. `RenderedPresentationSink` translates Window refreshes, Labels and Buttons into deterministic clipped drawing commands and can render TextFields when supplied with a `RenderedMeasurementContext`. `DisplayListExecutor` replays those commands through the SDL/native-free `RenderDevice` boundary. The concrete adapter now covers both headless SDL3 software rendering and a real window-backed SDL3 host. The window path reuses the existing semantic Widgets and Rendered pipeline, performs complete-frame replay, translates close/resize/key/committed-text events, and separates logical size from physical pixel size/display scale. It remains experimental and build-tree-only. Pointer events, hit-testing, handler-owned capture, Button press/release semantics and rendered TextField click-to-caret are now implemented; wider visible Windows/Linux/macOS validation and later richer pointer interactions are the next M3 work. Without usable caret metrics, a field deliberately remains `deferred` instead of being approximated silently.

## Build and test

A normal development build with tests:

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DSASD_UI_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

A stricter local build can turn warnings into errors:

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DSASD_UI_BUILD_TESTS=ON \
  -DSASD_UI_WARNINGS_AS_ERRORS=ON
```

On supported Clang/GCC environments, sanitizers can additionally be enabled:

```bash
cmake -S . -B build-sanitized \
  -DCMAKE_BUILD_TYPE=Debug \
  -DSASD_UI_BUILD_TESTS=ON \
  -DSASD_UI_WARNINGS_AS_ERRORS=ON \
  -DSASD_UI_ENABLE_SANITIZERS=ON
cmake --build build-sanitized --parallel
ctest --test-dir build-sanitized --output-on-failure
```

### Run the terminal demo

Examples are built by default (`SASD_UI_BUILD_EXAMPLES=ON`).

Linux/macOS with a single-config generator:

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DSASD_UI_BUILD_EXAMPLES=ON
cmake --build build --parallel
./build/examples/sasd_ui_terminal_demo
```

Windows with Visual Studio 2022:

```powershell
cmake -S . -B build-msvc -G "Visual Studio 17 2022" -A x64 -DSASD_UI_BUILD_EXAMPLES=ON
cmake --build build-msvc --config Debug --parallel
.\build-msvc\examples\Debug\sasd_ui_terminal_demo.exe
```

The demo requires an interactive terminal. Type a name, use Tab/Shift+Tab to move focus, activate buttons with Enter/Space, press F1 for help, resize the terminal, and press F10, Escape or the Exit button to leave. Terminal state is restored through RAII on normal exit and exceptions.

CI goes beyond compilation: Linux/macOS run the native POSIX adapter inside a real kernel PTY, while Windows runs the native adapter inside a real ConPTY pseudoconsole. These process-level tests verify native size discovery, raw/VT input, frame output and terminal-state restoration. The M2 manual release gate has additionally passed in a Linux/xterm-like environment and Windows Terminal.

The exact release-gate procedure and recorded results are documented in the [English M2 terminal smoke test](docs/en/M2_TERMINAL_SMOKE_TEST.md) and [German M2-Terminal-Smoke-Test](docs/de/M2_TERMINAL_SMOKE_TEST.md).

M3 visible desktop validation is tracked separately in the [English M3 rendered desktop smoke test](docs/en/M3_RENDERED_DESKTOP_SMOKE_TEST.md) and [German M3-Rendered-Desktop-Smoke-Test](docs/de/M3_RENDERED_DESKTOP_SMOKE_TEST.md).

### Build the experimental SDL3 adapter

The first SDL3 adapter is deliberately **opt-in and build-tree-only** while M3 validates its dependency,
window-lifecycle and packaging model. Normal toolkit builds do not need SDL.

To let CMake fetch the pinned SDL3/SDL_ttf versions used by the dedicated CI job:

```bash
cmake -S . -B build-sdl3 \
  -DCMAKE_BUILD_TYPE=Debug \
  -DSASD_UI_BUILD_TESTS=ON \
  -DSASD_UI_BUILD_EXAMPLES=ON \
  -DSASD_UI_BUILD_SDL3_ADAPTER=ON \
  -DSASD_UI_FETCH_SDL3=ON \
  -DSASD_UI_SDL3_TEST_FONT=/path/to/a/test-font.ttf
cmake --build build-sdl3 --parallel
ctest --test-dir build-sdl3 -R sasd_ui_sdl3_tests --output-on-failure
```

If compatible SDL3 and SDL3_ttf CMake packages are already installed, leave
`SASD_UI_FETCH_SDL3=OFF` and CMake uses `find_package` instead. The toolkit does not bundle a font;
the optional test font path is supplied by the developer/CI environment. Third-party dependency notes
are recorded in [`docs/third-party/`](docs/third-party/README.md).

### Install and consume as a CMake package

The current development line installs public headers, Core, Terminal and Rendered libraries plus CMake package metadata. The published v0.1.0 tag contains the Core/Terminal baseline; `main` now targets v0.2.0/M3. A typical Release installation is:

```bash
cmake -S . -B build-release \
  -DCMAKE_BUILD_TYPE=Release \
  -DSASD_UI_BUILD_TESTS=OFF \
  -DSASD_UI_BUILD_EXAMPLES=OFF
cmake --build build-release --parallel
cmake --install build-release --prefix /your/install/prefix
```

For Visual Studio/multi-config generators, select the configuration when building and installing:

```powershell
cmake -S . -B build-release -G "Visual Studio 17 2022" -A x64 `
  -DSASD_UI_BUILD_TESTS=OFF `
  -DSASD_UI_BUILD_EXAMPLES=OFF
cmake --build build-release --config Release --parallel
cmake --install build-release --config Release --prefix C:\path\to\sasd-ui
```

A consuming project can then use normal CMake package discovery:

```cmake
find_package(SASDUIToolkit 0.2 CONFIG REQUIRED)

# Platform-neutral Core only:
target_link_libraries(my_app PRIVATE SASD::UI)

# Terminal backend (pulls in Core transitively):
target_link_libraries(my_terminal_app PRIVATE SASD::UI::Terminal)

# Rendered-desktop command layer (SDL-independent M3 foundation):
target_link_libraries(my_rendered_app PRIVATE SASD::UI::Rendered)
```

Set `CMAKE_PREFIX_PATH` to the chosen install prefix when it is not in CMake's default search paths.
CI verifies this installed-package flow with a separate consumer project on GCC, Clang, AppleClang and MSVC.

## Architectural direction

```text
                     SASD Application
                           │
                     Public UI API
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
   Components            Layouts            Models
        │                  │                  │
        └──────────────────┼──────────────────┘
                           │
                      Events/Commands
                           │
                         Toolkit
                           │
                    Backend/Peer API
                           │
       ┌───────────────────┼────────────────────┐
       │                   │                    │
 Native Desktop       Rendered Desktop        Terminal
 Win32/GTK/AppKit        e.g. SDL3          ANSI/VT/Console
```

The public API should describe **what a UI element means**, while the backend decides **how that element is represented**.

A future `Button`, for example, may become a native Windows/GTK/AppKit control, a rendered button in an SDL-backed window, or an interactive text element in a terminal. Application logic should not have to be rewritten for each representation.

## Naming: the namespace carries the project name

Public classes deliberately do **not** repeat `SASD` or `Sasd` in every type name. Modern C++ namespaces already provide library identity and collision avoidance.

Preferred:

```cpp
sasd::ui::Window window;
sasd::ui::Button okButton{"OK"};
```

A local alias can make application code even shorter:

```cpp
namespace ui = sasd::ui;
ui::Window window;
ui::Button okButton{"OK"};
```

Names such as `SasdWindow`, `SasdButton` or `SasdObject` are intentionally not part of the naming strategy. A universal `Object` base class will also not be introduced merely to imitate VCL or Java; it would need a concrete technical justification.

## Design principles

### Component-oriented, but modern C++

The approachable component model of VCL remains a useful inspiration, but SASD UI Toolkit is planned around C++20, RAII, explicit ownership and normal C++ tooling.

```text
Component
├── Command
├── Action
├── Timer
├── DataSource
└── Widget
    ├── Label
    ├── Button
    ├── TextField
    └── Container
        ├── Panel
        └── Window
```

`Component` is a toolkit base for components that need component/lifecycle semantics; it is not intended to become an artificial root class for every value type in the library.

The M1 implementation also keeps **ownership** and **visual parenting** distinct: a non-visual component can have an owner without becoming a visual child.

### AWT-style abstraction, without becoming an AWT clone

Java AWT demonstrated the value of a common component hierarchy, layout managers, an event queue and platform-specific peers. SASD UI Toolkit uses these ideas as architectural input while avoiding Java-specific and legacy API constraints.

### Lightweight/rendered fallback

Swing demonstrates why not every widget needs its own native operating-system object. SASD backends should be able to mix native and rendered peers where appropriate.

### Model/View for large data

The project plans a Model/View-style architecture for lists, trees and tables so data-heavy applications do not need to create one UI object for every data item or cell.

### Terminal is part of the architecture

A terminal does not have pixels, native buttons or desktop window chrome. Instead of hiding this fact, the toolkit will use backend capabilities and backend-specific measurement while keeping shared semantics for layout, focus, commands and events.

### Headless before visible backends

M1 includes a deterministic **headless/mock backend**. It validates component trees, ownership, events, focus, layout/lifecycle behavior and backend contracts without requiring a terminal or window system. The terminal remains the first user-visible backend.

## Planned target environments

| Environment | Intended strategy | Status |
|---|---|---|
| Headless / Mock | Deterministic contract and core testing | Implemented foundation |
| Terminal / ANSI / VT | Rendered terminal backend | M2 complete; v0.1.0 release baseline validated |
| Windows | Rendered backend first, native Win32 peers later | Planned |
| Linux | Rendered backend first, native GTK peers later | Planned |
| macOS | Rendered backend first, native AppKit peers later | Planned |
| SDL3 | Optional rendered desktop adapter, hidden behind SASD API | M3 foundation in progress; deterministic DisplayList implemented |

## Roadmap at a glance

1. **M0 – Architecture and repository foundation** – complete enough to begin implementation.
2. **M1 – Core skeleton and headless validation** – complete; core contracts are validated headlessly across the compiler/OS matrix.
3. **M2 – Terminal Preview / v0.1.0** – complete; first user-visible backend with `Window`, `Label`, `Button`, `TextField`, `VBox`, `HBox`, focus, input and validated native terminal sessions.
4. **M3 – Rendered Desktop Preview / v0.2.0** – in progress; deterministic `SASD::UI::Rendered` DisplayList foundation is implemented before the optional SDL3 adapter.
5. **Later milestones** – more controls, commands/actions, Model/View widgets, native Win32/GTK/AppKit peers, desktop integration and designer-oriented metadata/tooling foundations.

A full visual designer/RAD environment is intentionally a **separate sister project**, not part of the toolkit core.

See the full [English roadmap](docs/en/ROADMAP.md) or [German roadmap](docs/de/ROADMAP.md).

## Documentation

Documentation is maintained in German and English.

### English

- [Goals and scope](docs/en/GOALS_AND_SCOPE.md)
- [Architecture](docs/en/ARCHITECTURE.md)
- [Backend and platform strategy](docs/en/BACKENDS.md)
- [Development guidelines](docs/en/DEVELOPMENT_GUIDELINES.md)
- [Roadmap](docs/en/ROADMAP.md)
- [v0.1.0 release notes](docs/en/V0_1_0_RELEASE_NOTES.md)
- [Inspirations and references](docs/en/REFERENCES.md)

### Deutsch

- [Projektziele und Umfang](docs/de/ZIELE_UND_UMFANG.md)
- [Architektur](docs/de/ARCHITEKTUR.md)
- [Backend- und Plattformstrategie](docs/de/BACKENDS.md)
- [Entwicklungsrichtlinien](docs/de/ENTWICKLUNGSRICHTLINIEN.md)
- [Roadmap](docs/de/ROADMAP.md)
- [v0.1.0-Release-Notes](docs/de/V0_1_0_RELEASE_NOTES.md)
- [Vorbilder und Referenzen](docs/de/REFERENZEN.md)

### Architecture decisions

- [Architecture Decision Records (ADR)](docs/adr/README.md)

The ADRs preserve not only **what** the current architecture is, but **why** major choices were made and which alternatives were rejected or deferred.

See also the [documentation index](docs/README.md).

## Architectural inspirations

| Project | Primary lesson for SASD UI Toolkit |
|---|---|
| Delphi VCL | Simple component model, events, ownership and developer productivity |
| Java AWT | Component/container hierarchy, event queue, layout managers and platform peers |
| Java Swing | Lightweight widgets and replaceable presentation |
| Qt | Model/View/Delegate, layouts and mature C++ application architecture |
| wxWidgets | Practical C++ portability with native controls |
| FLTK | Small, pragmatic toolkit design |
| FTXUI | Modern interactive terminal UI concepts |
| Turbo Vision | Component-oriented text-mode applications |
| SDL3 | Possible small cross-platform substrate for a rendered desktop backend |

These are references, not compatibility targets.

## Licensing

SASD UI Toolkit is released under the [MIT License](LICENSE).

The intent is to make the toolkit easy to use in open-source, research, private and commercial applications without license fees for the toolkit itself. Dependencies/data used by optional backends may have their own licenses and are documented explicitly; see `docs/third-party/` for current notices.

## Contributing

The project is at an early stage, so architecture discussions, small prototypes, tests and documentation improvements are especially valuable. Please read [CONTRIBUTING.md](CONTRIBUTING.md) before proposing implementation changes.

## Project philosophy

> Build the smallest coherent abstraction that can survive more than one backend.

A small toolkit with a clean core and six reliable widgets across terminal and desktop is more valuable than a large catalogue of partially portable controls.

---

**SASD UI Toolkit** is a SASD-GmbH open-source project initiated by Robin Goerlach.
