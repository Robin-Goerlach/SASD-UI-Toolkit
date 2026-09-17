# Goals and Scope

## Vision

The **SASD UI Toolkit** is intended to become a free, modern C++ library for user interfaces that allows applications to be developed against **one common API** and then run across different target environments.

Long-term target environments include:

- Windows desktop
- Linux desktop
- macOS desktop
- ANSI/VT-compatible terminals, including xterm-like environments
- modern Windows terminals, PowerShell and consoles with virtual-terminal support

The project is explicitly **not** intended as an exact reimplementation of Delphi VCL, Java AWT/Swing, Qt or wxWidgets. These frameworks are sources of architectural experience. SASD UI Toolkit should develop its own modern C++ architecture from those lessons.

## Primary goals

### 1. One public API for multiple presentation models

Application code should be largely independent of whether a widget is represented by a native operating-system control, a custom renderer or a terminal representation.

Illustrative target API:

```cpp
sasd::ui::Application app;
sasd::ui::Window window{"Example"};

window.setLayout(sasd::ui::VBox{});
window.add<sasd::ui::Label>("Hello from SASD UI Toolkit");
window.add<sasd::ui::Button>("Close");

return app.run(window);
```

This API is illustrative only and is not yet stable.

### 2. Platform-neutral core

The core must not require direct dependencies on Win32, AppKit, GTK, Qt or a terminal library. Platform-specific code belongs behind explicit backend interfaces.

### 3. Native controls where they are useful

Desktop backends should be able to use native controls when they provide meaningful platform behavior, input integration or accessibility benefits.

Native controls are not mandatory for every widget. A backend must also be able to provide rendered widgets where no suitable native control exists.

### 4. Terminal as a first-class backend

Terminal support is not an afterthought. Architecture, layout, focus navigation, events and capability discovery should be designed from the beginning to support text-based environments.

### 5. Modern C++ library

The initial language baseline is planned as **C++20**. Important goals are:

- RAII and explicit ownership
- type-safe APIs where practical
- standard library before unnecessary reinvention
- no mandatory macro-heavy or code-generator-heavy programming model
- small and testable interfaces
- understandable code before micro-optimization

### 6. Free use without SASD license fees

The project is MIT licensed. It should be usable in open-source and commercial applications without license fees for SASD UI Toolkit.

### 7. Fast and honest early releases

The first release should not be delayed by building a huge widget catalogue. Early releases should deliver small but coherent vertical slices: core, events, layout, a few essential widgets and at least one usable backend.

## Non-goals for early releases

The following are useful long-term topics but are not required for `v0.1.0`:

- feature parity with VCL, Swing, Qt or wxWidgets
- visual GUI designer
- IDE integration
- advanced docking
- rich text editor
- complete DataGrid/TreeView suites
- 3D graphics
- web backend
- mobile platforms
- ABI stability across early `0.x` releases

## Intended users

Long term, the toolkit is aimed at:

- C++ developers building classic desktop software
- administration and engineering tools
- applications that want GUI and terminal front ends from a shared structure
- scientific and technical applications
- SASD projects that need a common UI foundation
- open-source projects looking for a permissively licensed UI abstraction

## Success criteria

The architecture is succeeding when:

1. the same small sample application runs on at least two substantially different backends with unchanged application logic;
2. backend details do not leak into normal application code;
3. a new widget can be added without modifying every existing widget;
4. a new backend can be implemented without redesigning the core;
5. terminal and desktop backends can share semantic concepts for focus, layout and events;
6. documentation clearly distinguishes implemented functionality from planned functionality.
