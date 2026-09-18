# Backend and Platform Strategy

## Goal

SASD UI Toolkit should provide one semantic API without pretending that every target platform has identical capabilities. A backend translates widgets, events, layout measurements and platform services into a concrete environment.

## Backend types

### 1. Terminal backend

The terminal backend is planned as the first complete backend and as an architectural stress test.

Goals:

- ANSI/VT sequences where available;
- keyboard input and focus navigation;
- resize detection;
- 16/256/true-color depending on capabilities;
- optional mouse support where the terminal provides it;
- Unicode with progressively improving grapheme/display-width handling;
- no requirement for ncurses in the public API.

An implementation may use system-specific helpers or optional libraries internally as long as they remain behind the backend boundary.

### Current M2 implementation status

The first terminal building block now exists as a separately linkable `SASD::UI::Terminal` target. It contains a fully headless-testable off-screen `ScreenBuffer` abstraction for terminal cells. Rendering, clipping and resize rules can therefore be validated across every CI platform before a real console device is involved.

A `Cell` initially stores one Unicode code point as `char32_t`. This does **not** claim that every code point occupies exactly one terminal column. Wide characters, combining marks and grapheme clusters will be handled by a dedicated display-width/text layer in a later M2 step. The current representation merely avoids the much worse assumption that one UTF-8 byte equals one character equals one cell.

ANSI/VT output, real terminal I/O, alternate-screen/cursor control, styles/colors, display-width calculation and concrete presentation of `Window`, `Label`, `Button` and `TextField` are not implemented yet.

### 2. SDL3 backend

An SDL3 backend is planned as an early graphical proof of concept. It should demonstrate that the same component tree can be rendered in a desktop window across multiple platforms.

SDL3 would remain a **private/optional backend dependency**. Normal SASD UI application code must not require SDL types.

### 3. Native Windows backend

Long-term goal: map suitable widgets to native Win32/Windows controls and services.

Important areas include:

- native windows and controls;
- Windows message/event integration;
- clipboard;
- accessibility;
- high DPI;
- IME/text input;
- drag and drop.

### 4. Native Linux backend

A GTK-based native backend is a natural candidate for Linux. The exact GTK version should be selected when implementation begins.

GTK must not become part of the public core API.

### 5. Native macOS backend

An AppKit-based backend is planned for macOS. C++/Objective-C++ may be used inside the backend without leaking Objective-C++ into the public C++ API.

## Backend selection

Backends should be selectable at build time and, where practical, at runtime.

Conceptual package structure:

```text
sasd-ui-core
├── sasd-ui-terminal
├── sasd-ui-sdl
├── sasd-ui-win32
├── sasd-ui-gtk
└── sasd-ui-appkit
```

Applications should only need to link the backends they actually use.

## Fallback model

Not every SASD widget needs a native counterpart on every platform.

Preferred order:

1. native control where it semantically fits and provides a real platform benefit;
2. rendered peer where no suitable native control exists;
3. explicit unsupported capability when a feature cannot sensibly be provided.

Silently dropping important functionality is not a design goal.

## Capability discovery

Backends should report capabilities explicitly, including areas such as:

- pointer/mouse support;
- clipboard;
- drag and drop;
- multiple top-level windows;
- native menus;
- system tray;
- color depth;
- accessibility;
- touch/pen;
- IME;
- file dialogs.

Applications can then stay within a portable subset or intentionally enable optional platform features.

## Portability levels

Three levels are useful for documentation and testing:

### Portable Core

Features every supported backend must provide.

### Portable Extended

Features expected on primary desktop backends but allowed to be unavailable in constrained terminals.

### Backend Extension

Deliberately platform-specific features. These must be clearly marked and kept outside the portable API core.

## Why not make Qt or wxWidgets mandatory?

Both projects are valuable architectural references. SASD UI Toolkit should nevertheless remain an independent abstraction. Requiring a complete GUI framework underneath would:

- reduce control over backend design and terminal integration;
- increase project size and dependency surface;
- turn much of the public API into a wrapper over another framework.

This does not prevent optional integration or studying their implementations and design choices.

## Backend testing

Every backend should satisfy the same semantic contract tests where required by its capability level.

Examples:

- activating a button produces exactly one activation event;
- focus traversal follows the same semantics;
- `VBox` respects minimum sizes;
- disabled widgets do not trigger user actions;
- text input and key events remain distinct.

Each backend additionally receives platform-specific integration and smoke tests.
