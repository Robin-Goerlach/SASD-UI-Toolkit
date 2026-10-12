# ADR 0151 – M6 starts with a native host boundary and one Win32 consumer

**Status:** Accepted
**Date:** 2026-10-12
**Predecessors:** [0005](0005-backend-peer-and-capabilities.md), [0013](0013-presentation-coordinator-and-sink.md), [0031](0031-window-backed-sdl3-host-and-presentation-replay.md)

## English

### Context

M6 begins after the Core, Terminal and Rendered contracts have real consumers. The existing SDL3
window host proves that a desktop adapter can own a top-level native surface and translate input while
the semantic Widget tree remains in Core. Starting GTK, AppKit and a universal NativeWidget hierarchy
at the same time would freeze abstractions before a second consumer validates them.

### Decision

The first native contract is a small top-level host, implemented as the separately linkable
`SASD::UI::Native::Win32` target and exposed through `Win32WindowBackend`.

- Core owns `Application`, `Window`, Widgets, focus, commands, model/view, pointer routing and event
  dispatch. Native code must not duplicate Button, TextField or other widget semantics.
- `Backend` remains the lifecycle and semantic-event polling boundary. Win32 translates messages into
  the existing `KeyEvent`, `TextInputEvent`, `PointerEvent`, `PointerSurfaceEvent`, `ResizeEvent` and
  `QuitEvent` types. Native mouse leave retires surface hover/capture through that existing Core
  lifecycle event.
- A native host may feed either the existing Rendered presentation pipeline or a later native peer
  sink. This first slice is a host, not a native child-control implementation.
- Native handles are exclusively owned by the backend implementation, attached to the Win32 window
  with `WM_NCCREATE`/`GWLP_USERDATA`, and destroyed through one RAII shutdown path. No handle is
  exposed by Core or required by the portable host configuration.
- The backend is UI-thread affine: initialize, poll, native callbacks and destruction are required on
  the initializing thread. Unsupported/malformed messages are ignored rather than converted into
  invented Core semantics.
- IME composition, native Button/TextField peers, native menus, accessibility, DPI policy beyond
  the initial logical size, drag-and-drop and GTK/AppKit implementations remain deferred.

### Consequences

The public Core API stays free of Windows SDK types and existing Linux/macOS/Terminal/Rendered builds
remain Windows-SDK independent. The Win32 adapter can prove lifetime, message translation and Core
integration now. GTK and AppKit should first reuse the host/event boundary; only a concrete native
control need may justify a later peer contract.

## Deutsch

### Kontext

M6 beginnt erst, nachdem Core, Terminal und Rendered reale Consumer besitzen. Der bestehende SDL3-
Fensterhost zeigt, dass ein Desktop-Adapter eine Top-Level-Fläche besitzen und Eingaben übersetzen
kann, ohne den semantischen Widget-Baum aus dem Core zu duplizieren. GTK, AppKit und eine universelle
NativeWidget-Hierarchie gleichzeitig festzuschreiben, wäre vor der Validierung durch einen zweiten
Consumer verfrüht.

### Entscheidung

Der erste native Vertrag ist ein kleiner Top-Level-Host im separat linkbaren Target
`SASD::UI::Native::Win32` mit `Win32WindowBackend`.

- Core besitzt Application, Window, Widgets, Fokus, Commands, Model/View, Pointer-Routing und Event-
  Dispatch. Native Layer duplizieren keine Button-, TextField- oder andere Widget-Semantik.
- `Backend` bleibt die Grenze für Lifecycle und semantisches Event-Polling. Win32 übersetzt Nachrichten
  in die bestehenden Core-Typen `KeyEvent`, `TextInputEvent`, `PointerEvent`, `PointerSurfaceEvent`,
  `ResizeEvent` und `QuitEvent`. Native Maus-Leave-Ereignisse beenden über diesen bestehenden Core-
  Lifecycle-Vertrag Hover und Capture.
- Ein nativer Host kann die bestehende Rendered-Pipeline oder später einen nativen Peer-Sink speisen.
  Dieser erste Schnitt ist ein Host, keine native Child-Control-Implementierung.
- Native Handles gehören exklusiv dem Backend, werden über `WM_NCCREATE`/`GWLP_USERDATA` verbunden und
  über einen einzigen RAII-Shutdown-Pfad zerstört. Core und portable Host-Konfiguration sehen keinen
  Handle.
- Der Backend-Lifecycle ist UI-Thread-gebunden: Initialisierung, Polling, native Callbacks und
  Zerstörung erfolgen auf dem Initialisierungsthread. Nicht unterstützte oder fehlerhafte Nachrichten
  werden ignoriert statt in erfundene Core-Semantik übersetzt.
- IME-Komposition, native Button-/TextField-Peers, native Menüs, Accessibility, weitergehende DPI-
  Policy, Drag & Drop sowie GTK-/AppKit-Implementierungen bleiben zurückgestellt.

### Konsequenzen

Die öffentliche Core-API bleibt frei von Windows-SDK-Typen; Linux-, macOS-, Terminal- und Rendered-
Builds benötigen weiterhin kein Windows-SDK. Der Win32-Adapter kann Lifetime, Message-Translation und
Core-Integration jetzt beweisen. GTK und AppKit sollen zunächst dieselbe Host-/Event-Grenze validieren;
erst ein konkreter nativer Control-Bedarf rechtfertigt einen späteren Peer-Vertrag.
