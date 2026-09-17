# SASD UI Toolkit

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
![Project status](https://img.shields.io/badge/status-architecture%20%2F%20bootstrap-orange)
![C++ target](https://img.shields.io/badge/C%2B%2B-20-blue)

**Open-source C++ UI toolkit for building portable desktop and terminal applications across Windows, Linux and macOS.**

> **Current status:** architecture and repository bootstrap. There is no stable toolkit release yet. The repository currently documents the intended architecture before implementation starts.

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

A `Button`, for example, may become a native Windows/GTK/AppKit control, a rendered button in an SDL-backed window, or an interactive text element in a terminal. Application logic should not have to be rewritten for each representation.

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

### AWT-style abstraction, without becoming an AWT clone

Java AWT demonstrated the value of a common component hierarchy, layout managers, an event queue and platform-specific peers. SASD UI Toolkit uses these ideas as architectural input while avoiding Java-specific and legacy API constraints.

### Lightweight/rendered fallback

Swing demonstrates why not every widget needs its own native operating-system object. SASD backends should be able to mix native and rendered peers where appropriate.

### Model/View for large data

The project plans a Model/View-style architecture for lists, trees and tables so data-heavy applications do not need to create one UI object for every data item or cell.

### Terminal is part of the architecture

A terminal does not have pixels, native buttons or desktop window chrome. Instead of hiding this fact, the toolkit will use backend capabilities and backend-specific measurement while keeping shared semantics for layout, focus, commands and events.

### Headless before visible backends

Before the first real terminal or desktop backend, M1 includes a deterministic **headless/mock backend**. It exists to validate component trees, ownership, events, focus, layout and backend contracts without requiring a terminal or window system. The terminal remains the first user-visible backend.

## Planned target environments

| Environment | Intended strategy | Status |
|---|---|---|
| Headless / Mock | Deterministic contract and core testing | Planned for M1 |
| Terminal / ANSI / VT | Rendered terminal backend | Planned for first usable preview |
| Windows | Rendered backend first, native Win32 peers later | Planned |
| Linux | Rendered backend first, native GTK peers later | Planned |
| macOS | Rendered backend first, native AppKit peers later | Planned |
| SDL3 | Optional rendered desktop backend, hidden behind SASD API | Planned |

This table describes the intended direction, **not currently implemented support**.

## Early API direction

The following is intentionally illustrative and will change as the first prototype validates ownership, event and layout decisions:

```cpp
#include <sasd/ui/application.hpp>
#include <sasd/ui/button.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/layout/vbox.hpp>
#include <sasd/ui/window.hpp>

int main() {
    sasd::ui::Application app;
    sasd::ui::Window window{"SASD UI Toolkit"};

    window.setLayout(sasd::ui::VBox{});
    window.add<sasd::ui::Label>("Hello from SASD UI Toolkit");
    window.add<sasd::ui::Button>("Close");

    return app.run(window);
}
```

## Roadmap at a glance

The project intentionally starts small.

1. **M0 – Architecture and repository foundation**  
   Document scope, backend boundaries, design principles, ADRs and development rules.
2. **M1 – Core skeleton and headless validation**  
   CMake, `Application`, `Component`, `Widget`, `Container`, events, layout foundations, backend contracts, `BackendCapabilities`, deterministic mock backend, tests and CI.
3. **M2 – Terminal Preview / v0.1.0**  
   First user-visible backend with `Window`, `Label`, `Button`, `TextField`, `VBox`, `HBox`, focus and input.
4. **M3 – Rendered Desktop Preview / v0.2.0**  
   Demonstrate the same API graphically on Windows, Linux and macOS through an optional rendered backend.
5. **Later milestones**  
   More form controls, commands/actions, Model/View widgets, native Win32/GTK/AppKit peers, desktop integration and designer-oriented metadata/tooling foundations.

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
- [Inspirations and references](docs/en/REFERENCES.md)

### Deutsch

- [Projektziele und Umfang](docs/de/ZIELE_UND_UMFANG.md)
- [Architektur](docs/de/ARCHITEKTUR.md)
- [Backend- und Plattformstrategie](docs/de/BACKENDS.md)
- [Entwicklungsrichtlinien](docs/de/ENTWICKLUNGSRICHTLINIEN.md)
- [Roadmap](docs/de/ROADMAP.md)
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

The intent is to make the toolkit easy to use in open-source, research, private and commercial applications without license fees for the toolkit itself. Dependencies used by optional backends may have their own licenses and will be documented explicitly.

## Contributing

The project is at an early stage, so architecture discussions, small prototypes, tests and documentation improvements are especially valuable. Please read [CONTRIBUTING.md](CONTRIBUTING.md) before proposing implementation changes.

## Project philosophy

> Build the smallest coherent abstraction that can survive more than one backend.

A small toolkit with a clean core and six reliable widgets across terminal and desktop is more valuable than a large catalogue of partially portable controls.

---

**SASD UI Toolkit** is a SASD-GmbH open-source project initiated by Robin Goerlach.