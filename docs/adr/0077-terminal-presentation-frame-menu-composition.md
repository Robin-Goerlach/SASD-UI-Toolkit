# ADR 0077: Compose Terminal Menus Directly from Complete Presentation Frames

- Status: Accepted
- Date: 2026-10-03

## Context

ADR 0074 introduced `TerminalPresentationFrame` as the generic owned terminal presentation value containing a `ScreenBuffer` and optional hardware-caret position. ADR 0076 then added `TerminalPresentationSink::captureFrame()` so widget presentation can cross an explicit ownership boundary and produce that complete value without downstream code reconstructing it manually.

Menu composition still accepted the same two pieces separately through `composeMenuInteractionFrame(const ScreenBuffer&, std::optional<Point>, ...)`. A caller using `captureFrame()` therefore had to immediately dismantle the complete frame again into `frame.buffer` and `frame.caret` before entering the menu-composition stage.

That does not break correctness, but it weakens the value-oriented pipeline we deliberately introduced. It also makes callers know more than necessary about the internal payload of `TerminalPresentationFrame` and creates repeated call-site plumbing for a pairing that already has a named type.

## Decision

Add an overload of `composeMenuInteractionFrame()` that accepts `const TerminalPresentationFrame&` directly.

The overload does not implement menu composition a second time. It delegates to the existing buffer/caret primitive with `base.buffer` and `base.caret`. Therefore the established behavior remains authoritative for:

- copying the base buffer before painting transient menu chrome;
- deriving the viewport from the copied buffer size;
- suppressing the application caret while menu interaction is active;
- restoring the supplied caret when menu interaction becomes inactive;
- failing closed when menu state, text rendering, or placement is not representable completely.

The input `TerminalPresentationFrame` is immutable. The result is a new owned `TerminalPresentationFrame`, so menu composition remains a value-to-value transformation and never turns the captured application frame into retained mutable overlay state.

The older buffer/caret overload remains public. It is useful as the compatibility and primitive API, while the new overload is the preferred boundary when a complete frame already exists.

No optimization is introduced in this decision. The established implementation still performs the full `ScreenBuffer` copy required by the current correctness-first composition contract.

## Consequences

The normal terminal value pipeline can now remain typed end to end:

`TerminalPresentationSink::captureFrame() -> TerminalPresentationFrame -> composeMenuInteractionFrame(frame, ...) -> TerminalPresentationFrame -> TerminalSession::present(frame)`

Callers no longer need to dismantle and reconstruct the buffer/caret pair between presentation capture, transient menu composition, and transport.

Because the overload delegates rather than duplicating logic, future fixes to menu lifetime, caret suppression, or fail-closed behavior remain centralized in one implementation.

The extra overload increases public API surface slightly, but it removes repeated structural knowledge from callers and reinforces `TerminalPresentationFrame` as the backend-level value contract introduced by ADR 0074.

---

# ADR 0077: Terminal-Menüs direkt aus vollständigen Presentation-Frames zusammensetzen

- Status: Akzeptiert
- Datum: 2026-10-03

## Kontext

ADR 0074 führte `TerminalPresentationFrame` als allgemeinen eigenen Terminal-Presentation-Wert ein. Er enthält einen `ScreenBuffer` und eine optionale Hardware-Caret-Position. ADR 0076 ergänzte anschließend `TerminalPresentationSink::captureFrame()`, sodass die Widget-Präsentation über eine explizite Ownership-Grenze hinweg genau diesen vollständigen Wert erzeugen kann, ohne dass nachgelagerter Code ihn manuell zusammensetzen muss.

Die Menü-Komposition akzeptierte dieselben beiden Bestandteile weiterhin getrennt über `composeMenuInteractionFrame(const ScreenBuffer&, std::optional<Point>, ...)`. Ein Aufrufer von `captureFrame()` musste den gerade erzeugten vollständigen Frame daher unmittelbar wieder in `frame.buffer` und `frame.caret` zerlegen, bevor die Menü-Komposition aufgerufen werden konnte.

Das ist korrekt, schwächt aber die bewusst eingeführte wertorientierte Pipeline. Außerdem muss der Aufrufer dadurch mehr über den inneren Aufbau von `TerminalPresentationFrame` wissen als nötig und an mehreren Stellen dieselbe bereits benannte Paarung wieder auseinandernehmen.

## Entscheidung

`composeMenuInteractionFrame()` erhält einen Overload, der direkt `const TerminalPresentationFrame&` akzeptiert.

Dieser Overload implementiert die Menü-Komposition nicht ein zweites Mal. Er delegiert mit `base.buffer` und `base.caret` an das bestehende Buffer-/Caret-Primitiv. Damit bleibt die vorhandene Implementierung allein maßgeblich für:

- das Kopieren des Basisbuffers vor dem Zeichnen transienter Menüelemente;
- das Ableiten des Viewports aus der Buffer-Größe;
- das Unterdrücken des Anwendungs-Carets bei aktiver Menüinteraktion;
- das Wiederverwenden des gelieferten Carets bei inaktiver Menüinteraktion;
- fail-closed Verhalten, wenn Menüstatus, Textdarstellung oder Placement nicht vollständig repräsentiert werden können.

Der übergebene `TerminalPresentationFrame` bleibt unveränderlich. Das Ergebnis ist ein neuer eigener `TerminalPresentationFrame`. Die Menü-Komposition bleibt damit eine Wert-zu-Wert-Transformation und macht aus dem erfassten Anwendungsframe keinen gehaltenen veränderlichen Overlay-Zustand.

Der bisherige Buffer-/Caret-Overload bleibt öffentlich bestehen. Er bleibt als kompatible und primitive API sinnvoll; der neue Overload ist jedoch die bevorzugte Grenze, wenn bereits ein vollständiger Frame vorliegt.

Diese Entscheidung führt keine Optimierung ein. Die bestehende Implementierung kopiert weiterhin den vollständigen `ScreenBuffer`, wie es der aktuelle korrektheitsorientierte Kompositionsvertrag vorsieht.

## Konsequenzen

Der normale Terminal-Wertfluss kann nun durchgehend typisiert bleiben:

`TerminalPresentationSink::captureFrame() -> TerminalPresentationFrame -> composeMenuInteractionFrame(frame, ...) -> TerminalPresentationFrame -> TerminalSession::present(frame)`

Aufrufer müssen das Buffer-/Caret-Paar zwischen Presentation-Capture, transienter Menü-Komposition und Transport nicht mehr auseinandernehmen und erneut zusammensetzen.

Da der neue Overload delegiert und keine zweite Logik einführt, bleiben spätere Korrekturen an Menü-Lebensdauer, Caret-Unterdrückung oder fail-closed Verhalten an genau einer Stelle zentralisiert.

Der zusätzliche Overload vergrößert die öffentliche API geringfügig, entfernt dafür aber wiederholtes Strukturwissen aus dem Anwendungscode und stärkt `TerminalPresentationFrame` als backendweiten Wertvertrag aus ADR 0074.
