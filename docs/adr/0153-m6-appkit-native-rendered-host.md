# ADR 0153 – M6 AppKit validates the third native rendered-host consumer

**Status:** Accepted
**Date:** 2026-10-13
**Predecessor:** [0152](0152-m6-gtk-native-rendered-host.md)

## English

AppKit is the third M6 native host and directly implements `Backend`, `RenderDevice` and
`RenderedMeasurementContext` in `SASD::UI::Native::AppKit`. A private `NSWindow` and flipped custom
`NSView` host the existing Core `Window`, `Label`, `Button` and `TextField` presentation. There is no
native-widget hierarchy and no NSButton/NSTextField peer in this slice; Core remains authoritative
for semantics, focus, routing and commands.

Objective-C++ is enabled only for the opt-in `SASD_UI_BUILD_APPKIT_ADAPTER` target. Public headers
are ordinary C++20 and expose only a PImpl. AppKit, Foundation, CoreGraphics and CoreText are private
target dependencies. The backend uses `[NSApplication sharedApplication]` as a process-wide host
service and never terminates it; it owns only its window, view, delegate, queue and frame.

All lifecycle, native callbacks and polling are main-thread-affine and fail closed on another thread.
`pollEvent()` pumps a bounded non-blocking AppKit queue with `nextEventMatchingMask`, `sendEvent` and
`updateWindows` semantics without calling `[NSApp run]`. The last complete copied `DisplayList` is
replayed by `DisplayListExecutor` from draw callbacks and after resize/backing changes.

AppKit view points are SASD logical units. Backing scale is a pixel-store detail and is never applied
to layout, pointer or DisplayList coordinates a second time. CoreGraphics draws rectangles and clips;
CoreText supplies both measurement and text drawing. UTF-8 scalar boundaries are explicitly mapped to
UTF-16 indices before asking CoreText for offsets. `NSTextInputClient` supports committed UTF-8 text;
marked/preedit IME remains deferred and `ime` is not advertised.

The adapter is build-tree-only for now. The dedicated macOS workflow compiles the real Objective-C++
target and runs headless CoreGraphics/CoreText plus hidden-host lifecycle tests. The manual visible
smoke test remains the M3 macOS/Cocoa gate; headless CI does not close that gate.

## Deutsch

AppKit ist der dritte Native Host in M6 und implementiert in `SASD::UI::Native::AppKit` direkt
`Backend`, `RenderDevice` und `RenderedMeasurementContext`. Ein privates `NSWindow` und ein geflippter
Custom-`NSView` hosten die bestehende Core-Presentation von `Window`, `Label`, `Button` und `TextField`.
Es gibt in diesem Schnitt weder eine native Widget-Hierarchie noch NSButton-/NSTextField-Peers; Core
bleibt für Semantik, Fokus, Routing und Commands maßgeblich.

Objective-C++ wird nur für das optionale Target `SASD_UI_BUILD_APPKIT_ADAPTER` aktiviert. Öffentliche
Header bleiben normales C++20 und verwenden PImpl. AppKit, Foundation, CoreGraphics und CoreText sind
private Target-Abhängigkeiten. Der Backend nutzt `[NSApplication sharedApplication]` als prozessweiten
Host-Service und beendet ihn nie selbst; er besitzt nur eigenes Fenster, View, Delegate, Queue und Frame.

Lifecycle, native Callbacks und Polling sind an den Main Thread gebunden und schlagen bei einem
anderen Thread fehl. `pollEvent()` pumpt begrenzt und nichtblockierend mit
`nextEventMatchingMask`/`sendEvent`, ohne `[NSApp run]` aufzurufen. Die letzte vollständige kopierte
`DisplayList` wird aus Draw-Callbacks per `DisplayListExecutor` auch nach Resize/Backing-Änderungen
erneut abgespielt.

AppKit-View-Points sind SASD-Logikeinheiten. Die Backing Scale ist nur ein Pixel-Store-Detail und wird
nicht nochmals auf Layout, Pointer oder DisplayList angewendet. CoreGraphics übernimmt Rechtecke und
Clipping; CoreText liefert Messung und Textdarstellung. UTF-8-Scalar-Grenzen werden explizit auf
UTF-16-Indizes abgebildet, bevor CoreText Offsets liefert. `NSTextInputClient` unterstützt committed
UTF-8-Text; markierter/preedit IME bleibt zurückgestellt und `ime` wird nicht gemeldet.

Der Adapter bleibt zunächst build-tree-only. Die dedizierte macOS-Workflow kompiliert echtes
Objective-C++ und testet headless CoreGraphics/CoreText sowie den versteckten Host-Lifecycle. Der
manuelle sichtbare Smoke-Test bleibt das M3-macOS-/Cocoa-Gate; headless CI schließt dieses Gate nicht.
