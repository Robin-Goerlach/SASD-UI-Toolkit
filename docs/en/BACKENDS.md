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

Deferred by this slice: IME composition, native child controls, accessibility, drag and drop, advanced
DPI policy and visible GUI automation. Unsupported native messages fail closed.

#### Win32 native menu consumer (M7 slice 1)

`Win32WindowBackend::syncMenuBar()` explicitly projects the Core `MenuBarModel` into a private owned
HMENU tree; `clearMenuBar()` removes it. The projection includes recursive submenus, separators,
command labels, shortcut display text and enabled state. Structural changes require another explicit
synchronization call, while `WM_INITMENU`/`WM_INITMENUPOPUP` refresh live command state.

Windows owns popup navigation, highlighting, keyboard navigation, submenu opening and Escape behavior.
The native path therefore does not use `MenuInteractionController`, which remains the transient
interaction owner for SASD-rendered Terminal/Rendered menus. `WM_COMMAND` resolves a private
lifetime-safe `Command::Reference` and queues an application-level `CommandActivationEvent`; the
native callback then ends. `Application` exposes that event to its application handler without
implicit execution, so the host can stabilize presentation state and explicitly execute the command
later. Literal SASD ampersands are stored as `&&` in native labels because this slice has no mnemonic
model. Menu replacement builds a separate binding, checks the direct `SetMenu`/`DrawMenuBar` return
values, rolls back when possible and only commits ownership after installation succeeds. Structural
resynchronization remains explicit; command text/enabled refresh remains owned by `WM_INITMENU*`.
No Win32-specific semantic menu model or public cross-platform native-menu base class is introduced.
#### Win32 Open File dialog consumer (M7 file-dialog slice 1)

`Backend::fileDialogs()` is an optional host-service accessor. The portable service accepts an
`OpenFileDialogRequest` and returns a non-zero `FileDialogRequestId`; completion is always queued as
the application-level `FileDialogResultEvent`, never returned as a path or delivered through a
callback. A Win32 request may synchronously enter the native modal loop, but application code is not
called inside it. Version 1 permits one pending request only.

The Win32 service uses `IFileOpenDialog` with `FOS_FORCEFILESYSTEM`, `FOS_FILEMUSTEXIST`,
`FOS_PATHMUSTEXIST` and `FOS_NOCHANGEDIR`, owned by the existing `Win32WindowBackend` HWND. Titles are
strictly converted from UTF-8 to UTF-16. Accepted results retain the native path as a
`std::filesystem::path` without canonicalization or lossy ACP conversion; cancel is a pathless
`cancelled` result, and technical failures are pathless `failed` results. COM is initialized and
balanced only for the calling UI-thread dialog operation. Filters, save, folder and multiple
selection are intentionally not part of this first contract. See [ADR 0158](../adr/0158-m7-file-dialog-request-result-and-win32-open-file.md).
#### AppKit Open File dialog consumer (M7 file-dialog slice 2)

`AppKitWindowBackend::fileDialogs()` exposes the same optional Core service as Win32. AppKit starts
`NSOpenPanel` with `canChooseFiles` enabled, directories and multiple selection disabled, and uses
`beginSheetModalForWindow:completionHandler:` against the backend's private `NSWindow`. The request
ID returns before the sheet completes; exactly one application-level result is queued later. Pending
ends when that result has been queued, not when the application consumes it, so another request can
start immediately afterward.

The private invoker seam keeps tests headless. A shared, invalidatable completion state holds only the
queue and request identity; native blocks capture a weak state and therefore cannot retain the
backend, service or queue. Generation and exactly-once checks reject stale and duplicate completions,
and shutdown invalidates the state before canceling the panel. Titles use strict UTF-8 conversion;
accepted results use the `NSURL` file-system representation without canonicalization, normalization or
post-selection existence checks. The native completion only queues `FileDialogResultEvent`; application
callbacks run later through the normal event-polling path.

Manual macOS smoke test: start `native_appkit_form_demo`, choose `File -> Open File`, verify that the
sheet belongs to the SASD window, permits files only and single selection, then cancel and open again.
Select a Unicode-named file and verify that output appears only after the completion event and that the
window remains usable. This visible test is not run on Linux CI.

#### GTK4 Open File dialog consumer (M7 file-dialog slice 3)

`GtkWindowBackend::fileDialogs()` exposes the same optional `FileDialogService` while the existing
`GtkWindow` is initialized. The production adapter uses `GtkFileDialog` when the discovered GTK
headers provide GTK 4.10 or newer, and otherwise uses the compatible asynchronous
`GtkFileChooserNative` response API. This conditional choice preserves the repository's existing
unpinned GTK4 minimum; no second `GtkApplication`, main loop, window or event queue is introduced.

The dialog belongs to the existing `GtkWindow`, selects exactly one existing file, and never returns a
directory or multiple selection. Completion is copied into a `FileDialogResultEvent` and queued through
the same `EventQueue` that `pollEvent()` drains by pumping the GLib default context. At most one native
operation is pending; generation/exactly-once checks reject stale and duplicate callbacks, and
shutdown invalidates state before canceling and releasing GTK objects. Native callbacks do not execute
commands or application callbacks.

Accepted `GFile` results use `g_file_get_path()` and an immediately owned `std::filesystem::path`;
non-local URIs, empty paths and technical errors become `failed`. No canonicalization, normalization,
existence check, save/folder/multiple/filter policy or public GTK dialog type is added. Tests use a
private invoker seam and never display a chooser. See [ADR 0160](../adr/0160-m7-gtk4-open-file-dialog.md)
and the three-consumer review in [ADR 0161](../adr/0161-m7-file-dialog-three-consumer-contract-review.md).

Manual GTK smoke test: start `native_gtk_form_demo`, choose `File -> Open File`, select a file while
checking single selection and that directories cannot be returned, then cancel and open again. Select
a filename containing spaces or non-ASCII text and verify output appears only after the queued result
event and the main window remains usable. This visible test is not claimed as run by Linux/Xvfb CI.

#### M7 Save File across the native consumers (slice 4)

The same `FileDialogService` now also accepts `SaveFileDialogRequest` through `requestSaveFile()`. Open
and Save share request IDs, one pending-operation boundary and queued `FileDialogResultEvent`
completion. An accepted result is a local filesystem destination that may already exist or may be new;
the toolkit does not create, open, truncate, write, canonicalize or existence-check it. The native
dialog owns overwrite confirmation, and no filters or suggested filename are exposed yet.

Win32 uses `IFileSaveDialog` owned by the existing `HWND`, synchronously like Open File, with
`FOS_FORCEFILESYSTEM`, `FOS_PATHMUSTEXIST`, `FOS_NOCHANGEDIR` and `FOS_OVERWRITEPROMPT`; it deliberately
does not set `FOS_FILEMUSTEXIST`. AppKit uses an asynchronous `NSSavePanel` sheet owned by the existing
`NSWindow`. GTK uses `gtk_file_dialog_save()`/`save_finish()` with GTK 4.10+ and the asynchronous
`GtkFileChooserNative` `ACTION_SAVE` fallback for older supported GTK4 headers. All three copy the
native filesystem representation into `std::filesystem::path`, preserve Unicode/space characters,
and deliver completion through the existing EventQueue. Manual smoke testing should select both a new
and an existing target, observe native overwrite UX, cancel, and verify that no file was created by
the demo; this visible test is not claimed as run by CI.

#### M7 Folder Selection across the native consumers (slice 5)

`FileDialogService::requestSelectFolder(SelectFolderDialogRequest)` selects exactly one existing
local filesystem directory and reuses `FileDialogResultEvent`. Open, Save and Folder share request
IDs, one pending boundary, the EventQueue completion boundary and lifecycle/generation/
exactly-once protection. The operation never creates a directory, writes a path or changes the
process working directory. Filters, initial directories and multiselect remain deferred.

Win32 uses `IFileOpenDialog` with `FOS_PICKFOLDERS`, `FOS_FORCEFILESYSTEM`, `FOS_PATHMUSTEXIST` and
`FOS_NOCHANGEDIR`, owned by the existing `HWND`, and copies the UTF-16 `SIGDN_FILESYSPATH` result
directly to `std::filesystem::path`. AppKit uses an asynchronous `NSOpenPanel` sheet owned by the
existing `NSWindow`, configured for directories only and single selection. GTK 4.10+ uses
`gtk_file_dialog_select_folder()`/`select_folder_finish()`; older supported GTK4 uses the
asynchronous `GtkFileChooserNative` `ACTION_SELECT_FOLDER` fallback. GTK accepts only a local,
non-empty `g_file_get_path()` result. Native selection semantics establish directory existence; no
second filesystem existence check or canonicalization is performed.

Manual folder smoke test: start a native demo, choose `File -> Select Folder`, verify the dialog is
owned by the main window and files cannot be returned, select an existing directory, then test
Cancel and repeat with spaces/non-ASCII characters. Verify the queued result shows the directory,
the window remains usable, Open and Save still work, and the process working directory is unchanged.
This visible smoke test is not claimed as run by CI.

#### AppKit native menu consumer (M7 slice 2)

`AppKitWindowBackend::syncMenuBar()` builds a complete owned `NSMenu` tree from the same Core
`MenuBarModel` and installs it as the process-global `NSApplication.sharedApplication.mainMenu`.
Only one SASD AppKit menu owner is supported: a second backend fails closed, while an existing
process menu is saved as the restoration target and is replaced transactionally. `clearMenuBar()`
restores that previous menu only while the SASD menu is still current; the previous menu remains owned
by its original process owner.

AppKit labels are strict UTF-8, including CJK and supplementary Unicode; malformed input aborts the
complete replacement before the old menu changes. Structural changes require explicit resynchronization.
`menuNeedsUpdate`/`validateMenuItem:` refresh copied command text and enabled state. Native actions copy
only `Command::Reference` and queue `CommandActivationEvent`; no Command callback runs in the
Objective-C action. `keyEquivalent` remains empty because ShortcutMap is authoritative, and AppKit
mnemonics are not emulated. No universal NativeMenu abstraction is introduced yet. GTK remains the
next validation consumer.

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

#### GTK4 native menu consumer (M7 slice 3)

`GtkWindowBackend::syncMenuBar()` projects the same Core `MenuBarModel` into a private, window-local
`GMenu`/`GMenuModel` tree, `GSimpleActionGroup` and `GtkPopoverMenuBar`. The GTK root is a private
vertical `GtkBox`; the existing `GtkDrawingArea` remains the Rendered surface below the menu, so
`windowSize()`, resize events and pointer coordinates continue to use the drawing-area allocation.
Installing or clearing a menu never recreates the drawing area or its retained `DisplayList`.

The projection is owned and structural: model changes require explicit resynchronization, while live
`Command::text()` and `Command::isEnabled()` state is refreshed from `pollEvent()`. GTK actions use
private numeric-derived names and retain only `Command::Reference`; activation validates the reference
and queues `CommandActivationEvent` without calling `Command::execute()` in the GLib callback. Expired,
disabled and malformed dynamic text fail closed. GTK labels are strict UTF-8, preserve literal `&` and
`_`, and do not emulate Win32 mnemonics. Shortcut metadata is not registered as GTK accelerators;
`ShortcutMap` remains the authoritative input route. See [ADR 0156](../adr/0156-m7-gtk4-native-menu-consumer.md)
and [ADR 0157](../adr/0157-native-menu-portability-boundary.md).

### 5. Native macOS backend

AppKit is the optional third M6 native host, exposed in the build tree as
`SASD::UI::Native::AppKit` when `SASD_UI_BUILD_APPKIT_ADAPTER=ON` on macOS. `AppKitWindowBackend`
directly implements `Backend`, `RenderDevice` and `RenderedMeasurementContext`. A private flipped
`NSView` replays the complete owned `DisplayList` through CoreGraphics/CoreText; no NSButton or
NSTextField peers are created. AppKit view points are the same logical units used by Rendered, so a
backing-scale change requests repaint but never doubles layout or pointer coordinates.

The adapter owns its `NSWindow`, view, delegate, queue and last frame, but uses the process-wide
`NSApplication` service without terminating it. Lifecycle and callbacks are main-thread-affine.
`pollEvent()` performs a bounded non-blocking AppKit pump. `NSTextInputClient` delivers committed UTF-8
text separately from Core KeyEvents; marked/preedit IME is deferred. CoreText receives an explicit
UTF-8-scalar to UTF-16 mapping for caret advances. The target is build-tree-only until an installed
framework consumer is validated. See [ADR 0153](../adr/0153-m6-appkit-native-rendered-host.md) and
`tools/m6_smoke_appkit_macos.sh` for the visible macOS procedure. This visible test remains the open
M3 macOS/Cocoa gate.

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
