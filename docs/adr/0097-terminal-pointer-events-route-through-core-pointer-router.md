# ADR 0097 – Terminal pointer events route through Core PointerRouter

**Status:** Accepted  
**Date:** 2026-10-04

## English

### Context

ADR 0095 introduced decoding of xterm SGR-1006 mouse reports into backend-neutral `PointerEvent` values. ADR 0096 then gave `TerminalSession` explicit RAII ownership of the terminal protocol modes that request button-event tracking (`?1002`) and SGR coordinates (`?1006`).

At that point the terminal backend could both request and decode pointer reports, but one architecture question remained: should terminal applications introduce a terminal-specific pointer router, or should decoded events enter the existing Core interaction path used by rendered desktop backends?

The toolkit already has backend-neutral `HitTest` and `PointerRouter`. Widget bounds in the terminal presentation path are expressed in the same logical coordinate type as SGR coordinates after the decoder converts them from one-based terminal cells to zero-based SASD UI coordinates. A second routing model would therefore duplicate hit testing, capture ownership and Widget dispatch without adding information.

Production hosts also need a convenient way to opt into terminal pointer reporting while still using the native POSIX/Windows terminal device. Requiring every application to call `createNativeTerminalDevice()` merely to set `TerminalSessionOptions::pointer_input` would leak an unnecessary construction detail into ordinary host code.

### Decision

Decoded terminal `PointerEvent` values are routed through the existing Core `PointerRouter`. No terminal-specific pointer router is introduced.

The contract is:

1. `TerminalSession` owns protocol enable/disable lifetime only.
2. `AnsiInputDecoder` converts SGR transport bytes and one-based terminal-cell coordinates into backend-neutral `PointerEvent` values with zero-based logical coordinates.
3. `TerminalBackend` exposes those values through the normal `Backend::pollEvent()` contract without changing their type.
4. Application/host code routes each terminal `PointerEvent` exactly once through Core `PointerRouter` against the current Widget root.
5. `PointerRouter` remains the sole owner of geometric hit testing, bubbling, hover state and handler-acquired capture.
6. Control behavior after routing is backend-neutral. A Button driven by terminal SGR press/release events therefore follows the same semantic press/capture/release/activation path as a Button driven by a rendered desktop backend.
7. Focus-on-primary-press remains host/focus-scope policy. `PointerRouter` does not implicitly change `FocusManager` state.
8. Terminal-specific TextField caret/selection geometry remains separate. Generic routing can establish capture, but mapping a terminal cell to a Unicode scalar/caret is a presentation-specific policy to be added in a later terminal TextField interaction slice.

`TerminalBackend` gains a native-device convenience constructor taking `TerminalSessionOptions` and `TerminalEventPumpOptions`. It delegates to the existing injected-device constructor, so device ownership, validation and lifecycle logic still have one implementation.

### Why no terminal-specific routing layer

After SGR decoding, the terminal has already crossed its backend boundary. The remaining facts are exactly the facts Core understands: logical position, pointer action, button, click count and modifiers.

Adding `TerminalPointerRouter` would create two implementations of rules that are intentionally backend-neutral:

- child hit testing and deepest-target selection;
- event bubbling;
- press-handler capture acquisition;
- captured move/release delivery;
- capture retirement.

That duplication would make terminal behavior drift from rendered/native backends over time. Reusing `PointerRouter` instead makes terminal support a validation of the common architecture rather than a parallel UI stack.

### Validation

A deterministic terminal backend test now exercises the complete semantic chain without a real TTY:

`MockTerminalDevice` -> SGR bytes -> `TerminalEventPump` -> `AnsiInputDecoder` -> `TerminalBackend::pollEvent()` -> `PointerRouter` -> real Core `Button`.

The test verifies that a one-based SGR press inside the Button becomes the expected zero-based logical point, acquires normal pointer capture, and that the corresponding SGR release activates the Button exactly once and retires capture.

### Consequences

- terminal pointer input now has a proven end-to-end route into existing Core control semantics;
- no backend-specific pointer event or capture model is added;
- production native terminal hosts can opt into pointer reporting without constructing a native device explicitly;
- focus policy stays outside both backend and pointer router;
- the next terminal interaction work can concentrate on TextField cell-to-caret/selection geometry rather than rebuilding generic routing.

### Deferred scope

This ADR does not yet add:

- terminal demo mouse wiring;
- terminal TextField click/drag selection geometry;
- terminal double-/triple-click synthesis;
- menu-bar or popup mouse interaction;
- wheel events or extended buttons;
- pointer hover tracking without a pressed button (`?1003`).

---

## Deutsch

### Kontext

ADR 0095 hat die Dekodierung von xterm-SGR-1006-Mausmeldungen in backend-neutrale `PointerEvent`-Werte eingeführt. ADR 0096 hat anschließend `TerminalSession` die explizite RAII-Verantwortung für die Terminal-Protokollmodi gegeben, die Button-Event-Tracking (`?1002`) und SGR-Koordinaten (`?1006`) anfordern.

Damit konnte das Terminal-Backend Mausmeldungen sowohl anfordern als auch dekodieren. Eine Architekturfrage blieb jedoch offen: Sollten Terminalanwendungen einen terminalspezifischen Pointer-Router erhalten oder sollen dekodierte Events in denselben Core-Interaktionspfad eintreten, den auch gerenderte Desktop-Backends verwenden?

Das Toolkit besitzt bereits backend-neutrales `HitTest` und `PointerRouter`. Widget-Grenzen im Terminal-Presentation-Pfad verwenden denselben logischen Koordinatentyp wie SGR-Koordinaten, nachdem der Decoder sie von einsbasierten Terminalzellen in nullbasierte SASD-UI-Koordinaten umgerechnet hat. Ein zweites Routingmodell würde daher Hit-Testing, Capture-Besitz und Widget-Dispatch duplizieren, ohne zusätzliche Information zu liefern.

Produktive Hosts benötigen außerdem einen einfachen Weg, Terminal-Pointer-Reporting einzuschalten und trotzdem das native POSIX-/Windows-Terminalgerät zu verwenden. Jede Anwendung nur für `TerminalSessionOptions::pointer_input` zu `createNativeTerminalDevice()` zu zwingen, würde ein unnötiges Konstruktionsdetail in normalen Host-Code ziehen.

### Entscheidung

Dekodierte Terminal-`PointerEvent`-Werte werden durch den bestehenden Core-`PointerRouter` geroutet. Es wird kein terminalspezifischer Pointer-Router eingeführt.

Der Vertrag lautet:

1. `TerminalSession` besitzt ausschließlich die Lebensdauer der Protokoll-Aktivierung/-Deaktivierung.
2. `AnsiInputDecoder` übersetzt SGR-Transportbytes und einsbasierte Terminalzellen in backend-neutrale `PointerEvent`-Werte mit nullbasierten logischen Koordinaten.
3. `TerminalBackend` liefert diese Werte unverändert über den normalen `Backend::pollEvent()`-Vertrag aus.
4. Application-/Host-Code routet jedes Terminal-`PointerEvent` genau einmal über Core-`PointerRouter` gegen den aktuellen Widget-Root.
5. `PointerRouter` bleibt allein verantwortlich für geometrisches Hit-Testing, Bubbling, Hover-Zustand und durch Handler erworbenes Capture.
6. Control-Verhalten nach dem Routing bleibt backend-neutral. Ein Button, der von Terminal-SGR-Press/Release-Events angesteuert wird, durchläuft daher denselben semantischen Press-/Capture-/Release-/Activation-Pfad wie ein Button eines gerenderten Desktop-Backends.
7. Fokus bei Primary-Press bleibt Host-/Focus-Scope-Policy. `PointerRouter` verändert `FocusManager` nicht implizit.
8. Terminalspezifische TextField-Caret-/Selection-Geometrie bleibt getrennt. Generisches Routing kann Capture etablieren, aber die Abbildung einer Terminalzelle auf Unicode-Scalar/Caret ist Presentation-spezifische Policy und folgt in einem späteren Terminal-TextField-Interaction-Slice.

`TerminalBackend` erhält zusätzlich einen Native-Device-Komfortkonstruktor mit `TerminalSessionOptions` und `TerminalEventPumpOptions`. Er delegiert an den bestehenden Dependency-Injected-Konstruktor, sodass Device-Besitz, Validierung und Lifecycle-Logik weiterhin nur einmal implementiert sind.

### Warum keine terminalspezifische Routing-Schicht

Nach der SGR-Dekodierung ist die Terminal-Backend-Grenze bereits überschritten. Übrig bleiben genau die Fakten, die Core versteht: logische Position, Pointer-Aktion, Button, Click-Count und Modifier.

Ein zusätzlicher `TerminalPointerRouter` würde zwei Implementierungen für Regeln erzeugen, die bewusst backend-neutral sind:

- Child-Hit-Testing und Auswahl des tiefsten Ziels;
- Event-Bubbling;
- Capture-Übernahme durch den Press-Handler;
- Zustellung von Move/Release an das gecapturete Ziel;
- Beenden des Capture.

Diese Duplizierung würde dazu führen, dass sich Terminal- und Rendered-/Native-Verhalten mit der Zeit auseinanderentwickeln. Die Wiederverwendung von `PointerRouter` macht Terminalunterstützung stattdessen zu einer Bestätigung der gemeinsamen Architektur und nicht zu einem parallelen UI-Stack.

### Validierung

Ein deterministischer Terminal-Backend-Test durchläuft nun die komplette semantische Kette ohne reales TTY:

`MockTerminalDevice` -> SGR-Bytes -> `TerminalEventPump` -> `AnsiInputDecoder` -> `TerminalBackend::pollEvent()` -> `PointerRouter` -> echter Core-`Button`.

Der Test prüft, dass ein einsbasierter SGR-Press innerhalb des Buttons zum erwarteten nullbasierten logischen Punkt wird, normales Pointer-Capture übernimmt und dass der entsprechende SGR-Release den Button genau einmal aktiviert und Capture beendet.

### Folgen

- Terminal-Pointer-Input besitzt jetzt einen nachgewiesenen End-to-End-Pfad in vorhandene Core-Control-Semantik;
- es wird weder ein backend-spezifisches Pointer-Event- noch Capture-Modell ergänzt;
- produktive native Terminal-Hosts können Pointer-Reporting aktivieren, ohne ein natives Device explizit zu konstruieren;
- Fokus-Policy bleibt außerhalb von Backend und PointerRouter;
- die nächste Terminal-Interaction-Arbeit kann sich auf TextField-Zell-zu-Caret-/Selection-Geometrie konzentrieren, statt generisches Routing neu zu bauen.

### Bewusst später

Diese ADR ergänzt noch nicht:

- Mausverdrahtung im Terminal-Demo;
- Terminal-TextField-Klick-/Drag-Selection-Geometrie;
- Terminal-Double-/Triple-Click-Synthese;
- Mausinteraktion für Menüleiste oder Popups;
- Wheel-Events oder erweiterte Maustasten;
- Pointer-Hover-Tracking ohne gedrückten Button (`?1003`).
