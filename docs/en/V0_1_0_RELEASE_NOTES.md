# SASD UI Toolkit v0.1.0 – Release Notes

Status: prepared on 2026-09-28

v0.1.0 is the first usable vertical slice of SASD UI Toolkit. The release deliberately emphasizes
a small, testable architecture and a complete terminal path rather than a large widget catalogue.

## What v0.1.0 provides

- a platform-neutral C++20 Core with `Application`, `Component`, `Widget` and `Container`;
- explicit ownership and visual-parent relationships with RAII;
- event queue, routing/bubbling, focus management and Tab/Shift+Tab focus traversal;
- two-phase `measure()`/`arrange()` layout with backend-neutral `MeasurementContext`;
- `VBox` and `HBox` as the first automatic layout containers;
- `Window`, `Label`, `Button` and a single-line UTF-8 `TextField`;
- a separate `SASD::UI::Terminal` target with an off-screen `ScreenBuffer`;
- deterministic ANSI/VT output, text styling and terminal caret handling;
- non-blocking ANSI/VT input decoding including F1-F12;
- native POSIX terminal sessions and Windows console/ConPTY integration with RAII restoration;
- the interactive `sasd_ui_terminal_demo` sample application.

## Validation

The M2 release gate passed manually in a Linux/xterm-like environment and in Windows Terminal.
Validation covered focus, editing, resize, F1/F10, Escape, Exit-button paths and UTF-8 input using
`Robin AΩ界`.

CI additionally validates:

- Linux GCC;
- Linux Clang;
- macOS AppleClang;
- Windows MSVC;
- Linux Clang with ASan + UBSan;
- real POSIX PTY and Windows ConPTY process smoke tests;
- Release builds on Linux, macOS and Windows;
- staged installation and expected install contents;
- an external CMake consumer against the installed package.

## Consuming the CMake package

After installation a consumer can use normal CMake package discovery:

```cmake
find_package(SASDUIToolkit 0.1 CONFIG REQUIRED)

target_link_libraries(my_core_app PRIVATE SASD::UI)
target_link_libraries(my_terminal_app PRIVATE SASD::UI::Terminal)
```

`SASD::UI::Terminal` links the Core transitively.

## Deliberately not included yet

- mouse/pointer input;
- complete grapheme/emoji/ZWJ cell handling;
- bracketed paste and extended terminal keyboard protocols;
- RGB/true-color and background/theme cascade;
- rendered desktop backend;
- native Win32/GTK/AppKit peers;
- complete Model/View widgets and designer/RAD functionality.

## Compatibility status

v0.1.0 is a **pre-1.0 release**. The architecture is intentionally small and validated, but long-term
source compatibility is not frozen yet. Justified API corrections may still occur before 1.0 and
will remain documented through ADRs, the roadmap and the changelog.

## Publication status

These release notes are prepared for publication. The `v0.1.0` Git tag and GitHub Release will only
be published after explicit approval.
