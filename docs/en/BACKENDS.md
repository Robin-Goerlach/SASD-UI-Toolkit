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

The terminal layer now has a dedicated versioned `TextMetrics` abstraction. UTF-8 decoding is centralized; Narrow/Wide/Fullwidth values measure as one/two cells, and East-Asian-Ambiguous width is an explicit narrow/wide policy. `CellRole::wide_lead` and `wide_continuation` preserve two-column occupancy in the off-screen buffer. The currently generated tables are deliberately pinned to Unicode 17.0.0 and exposed as such through the API.

`Window` and `Label` now exist as the first semantic M2 widgets. A `TerminalPresentationSink` consumes their normal visual invalidation through `PresentationCoordinator` and renders deterministically into `ScreenBuffer`. UTF-8 is decoded safely, visual-parent offsets are resolved, widget/screen bounds are clipped, and malformed UTF-8 is replaced with U+FFFD. Unknown concrete widget types deliberately return `deferred` rather than silently losing their pending update.

Zero-width/combining/format sequences and ordinary terminal controls still cannot be preserved faithfully by the current simple Cell model. Those Label updates are therefore `deferred` before the buffer is modified, preserving the last successfully synchronized representation. The platform-neutral `Label` core still does not invent a terminal-specific intrinsic width. Full grapheme segmentation and an upgrade of the pinned Unicode data remain explicit M2 tasks.

Geometry changes and child removal now use a conservative subtree refresh: the terminal `Window` clears its off-screen buffer only for such geometry/structural damage, after which otherwise-clean descendants are deterministically replayed. Ordinary text/focus changes remain incremental. Dirty rectangles and region merging are deliberately later optimizations.

`Button` is now integrated as the first interactive control. Terminal measurement and presentation share the same fixed four-cell chrome: normal `[ caption ]`, focused `> caption <`, disabled `( caption )`. Enter/Space activates the focused Button on key press; this is intentionally terminal-friendly because ANSI/VT normally cannot provide reliable key-up events.

`TextField` is now integrated as a single-line editor. Terminal measurement reserves two delimiters plus one caret cell; when arranged narrower than its natural size, presentation scrolls horizontally on Unicode-scalar boundaries. The hardware caret is modeled as a separate optional position in `TerminalPresentationSink` and never overwrites a glyph cell. Combining/ZWJ content remains preserved in the core but is still `deferred` by the current simple Cell model.

`FocusTraversal` now provides deterministic Tab/Shift+Tab navigation across visible/enabled controls. In addition, `AnsiFrameEncoder` serializes the complete `ScreenBuffer` plus optional hardware caret into deterministic UTF-8/ANSI-VT frames. The encoder is intentionally I/O-free and manages neither raw mode, alternate screen nor OS handles.

`TerminalDevice`/`TerminalSession` now form the real device/session boundary. POSIX uses TTY detection, `termios`, `TIOCGWINSZ` and retry-safe `write`; Windows saves/restores console modes and code pages, enables Virtual Terminal processing and writes through `WriteFile`. Alternate screen and raw input are RAII-owned with transactional rollback. Hosted CI validates session semantics deterministically through `MockTerminalDevice` while native adapters are compiled on their respective platforms.

`TerminalDevice::readAvailable()` now returns currently available input bytes without blocking. `AnsiInputDecoder` incrementally handles split UTF-8/CSI/SS3 sequences and translates Enter, Tab, Backspace, Space, arrows, Home/End, Delete, PageUp/PageDown, Shift+Tab, logical Ctrl-letter C0 bytes and F1–F12, including selected xterm modifiers, into the existing semantic events. A lone ESC remains pending until an explicit event-loop flush. The terminal still reports no clipboard capability; Select All remains available as a semantic command without pretending that OSC52 or a system clipboard exists.

`TerminalEventPump` now combines size discovery, non-blocking input and incomplete-sequence timeout into deterministic semantic events. Size changes become the existing `ResizeEvent`; initial discovery does not synthesize a resize. `TerminalBackend : Backend` integrates session and event-pump lifecycle with the normal `Application` path. The runnable `sasd_ui_terminal_demo` exercises TextField, Buttons, Tab/Shift+Tab focus, resize and real ANSI output. CI compiles the demo on Linux, macOS and Windows; manual interactive smoke validation is still required for the M2 exit criterion.

`Color`/`TextStyle` now provide a deliberately small backend-neutral styling contract for `Label`, `Button` and `TextField`: 16 named foreground colors plus bold/dim/underline/inverse. Terminal cells retain resolved style and the ANSI encoder emits SGR transitions. Focus and disabled state are terminal-presentation overlays only. Background colors, cascade/themes, RGB/alpha and font models remain deliberately later.

F1–F12 are now backend-neutral `Key` identities decoded from common SS3/CSI/xterm forms. Extended keyboard protocols (F13+, Kitty/CSI-u/modifyOtherKeys), full grapheme/ZWJ presentation and pointer/hit-test interaction remain open.

### 2. Rendered desktop / experimental SDL3 adapter

M3 now has both a headless SDL3 proof and a real window-backed backend below the generic Rendered
layer. `SASD::UI::Rendered::SDL3` reuses one private SDL3 render/metric policy for software-surface
tests and desktop-window presentation.

`Sdl3WindowBackend` implements `Backend`, `RenderDevice` and
`RenderedMeasurementContext`. Window/Renderer/Font creation is transactional, complete DisplayList
frames are presented, close/resize/key/committed-text events are translated, and logical layout size
remains separate from physical pixel size/display scale. SDL types remain absent from Core and the
generic Rendered API.

`PresentationCoordinator::replay()` reconstructs a presentation surface from a clean semantic
Widget tree without exposing protected Widget invalidation.

SDL3 key events normalize logical A–Z keycodes to the backend-neutral `Key` identity while SDL text
input remains a separate `TextInputEvent` stream. The initialized window backend owns the concrete
UTF-8 SDL clipboard service and exposes it only as a non-owning `Backend::clipboard()` reference;
capability discovery becomes false again after shutdown.

Dedicated CI builds the SDL3 adapter and desktop demo, runs a hidden real SDL window through the
offscreen video driver, and exercises the end-to-end semantic Widget -> RenderedPresentationSink ->
DisplayList -> SDL3 window path.

The adapter remains opt-in and build-tree-only. Pointer/hit-testing, richer IME composition and wider
desktop validation remain later M3 slices.

### 3. Native Windows backend

The first M6 implementation is the separately linkable `SASD::UI::Native::Win32` target. Its
`Win32WindowBackend` owns one top-level HWND, consumes the generic Rendered `DisplayList` through
`RenderDevice`, supplies GDI-backed `RenderedMeasurementContext` metrics, and translates Win32 messages into the existing Core
`Backend`/`Event` pipeline. It is intentionally a native host, not a second Button/TextField
semantic system; the existing `EventDispatcher`, `FocusManager`, `PointerRouter`, Commands and
Model/View contracts remain authoritative. HWND ownership is RAII-managed and UI-thread-affine,
with `WM_NCCREATE`/`GWLP_USERDATA` used only inside the adapter. The host can later feed the existing
Rendered presentation path without a second widget semantic system. `WM_MOUSELEAVE` is translated to the
existing `PointerSurfaceEvent` contract so Core can retire hover/capture deterministically.

Win32 Rendered coordinates are logical 96-DPI units. The adapter converts client pixels, pointer
coordinates and its owned Segoe UI GDI font to/from the current window DPI. Resize and DPI changes
request complete replay; `WM_PAINT` replays the last owned DisplayList. GDI is intentionally the
small first text policy, not a generic font system or a DirectWrite commitment.

Manual smoke test on Windows: run `tools/m3_smoke_windows.ps1` for the existing host checks, then
start a small program that constructs a hidden/visible `Win32WindowBackend`, builds a frame with
`RenderedPresentationSink` and `PresentationCoordinator::replay()`, calls `presentFrame()`, resizes
the window, and verifies that the same semantic tree is replayed visibly after the resize and after
moving the window between monitors. This remains intentionally manual; the automated test is
headless and checks construction, DisplayList execution and GDI metrics without screenshot or GUI
automation dependencies.

Deferred by this slice: IME composition, native child controls, native menus, accessibility, drag and
drop, advanced DPI policy and visible GUI automation. Unsupported native messages fail closed.

Long-term goal: map suitable widgets to native Win32/Windows controls and services only where a real
platform benefit justifies a separately specified peer contract.

Important areas include:

- native windows and controls;
- Windows message/event integration;
- clipboard;
- accessibility;
- high DPI;
- IME/text input;
- drag and drop.

### 4. Native Linux backend

GTK4 is now an optional second native host. `SASD::UI::Native::GTK` is build-tree-only and uses
`GtkWindowBackend`, Cairo as `RenderDevice` and Pango as the shared measurement/drawing policy.
GTK is discovered with pkg-config only when `SASD_UI_BUILD_GTK_ADAPTER=ON`; normal builds remain
GTK-free. GLib is pumped from `pollEvent()` and complete DisplayLists are retained for redraw.
Committed printable input is supported; IME preedit remains deferred.

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

The terminal backend now has real process-level smoke tests rather than only mock/compiler coverage: Linux and macOS run the native POSIX adapter inside a kernel PTY, while Windows runs the native adapter inside a ConPTY pseudoconsole. The tests cover size discovery, raw/VT input, frame output, alternate-screen/cursor sequences and restoration of `termios` or Windows console modes/code pages. These automated pseudoterminal tests complement rather than replace manual emulator validation in Linux/xterm and Windows Terminal.
