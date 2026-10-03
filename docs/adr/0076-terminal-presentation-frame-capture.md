# ADR 0076: Capture Owned Terminal Presentation Frames from the Presentation Sink

- Status: Accepted
- Date: 2026-10-03

## Context

`TerminalPresentationSink` renders semantic widgets into a caller-owned mutable `ScreenBuffer` and separately exposes the current optional hardware-caret position. ADR 0074 introduced the generic owned `TerminalPresentationFrame`, and ADR 0075 made `TerminalSession` capable of presenting that complete frame value.

A small ownership gap remains between those two layers. Callers that want an owned frame currently have to know that the complete terminal presentation state is exactly `sink.buffer()` plus `sink.caretPosition()`, construct the frame manually, and decide themselves whether they are creating an alias or an independent snapshot.

That duplication becomes more important now that transient overlays such as menus can use a complete application frame as immutable base content while the presentation sink continues to reuse its mutable work buffer for subsequent widget synchronization passes.

## Decision

Add `TerminalPresentationSink::captureFrame() const`.

The method returns a `TerminalPresentationFrame` containing:

- an owned copy of the sink's current `ScreenBuffer`;
- a copy of the current optional hardware-caret position.

The returned frame is an ownership boundary, not a view. Later changes to the sink's working buffer or caret do not affect an already captured frame, and changes to the captured frame do not affect the sink.

The sink's `AmbiguousWidthMode` is intentionally not stored in `TerminalPresentationFrame`. Ambiguous-width handling is a policy used while measuring and producing terminal cell geometry. Once cells and caret coordinates have been captured, the frame represents the presentation result rather than the production policy.

The capture operation may allocate because it copies the complete `ScreenBuffer`. Allocation failures remain ordinary exceptions. No new retained history or synchronization mechanism is introduced.

## Consequences

The terminal presentation pipeline now has an explicit value transition from mutable incremental widget presentation to an owned complete frame:

`PresentationCoordinator -> TerminalPresentationSink -> captureFrame() -> TerminalPresentationFrame`

That frame can then be composed with menus or other overlays and finally passed to `TerminalSession::present(frame)` without requiring downstream code to reconstruct the buffer/caret contract manually.

The initial implementation intentionally performs a full buffer copy. This favors clear ownership and lifetime semantics over optimization. A later optimization phase may introduce move/reuse or damage-aware capture while preserving the observable contract that a captured frame is independent from subsequent sink mutations.

---

# ADR 0076: Eigene Terminal-Presentation-Frames aus dem Presentation-Sink erfassen

- Status: Akzeptiert
- Datum: 2026-10-03

## Kontext

`TerminalPresentationSink` rendert semantische Widgets in einen vom Aufrufer bereitgestellten veränderlichen `ScreenBuffer` und stellt zusätzlich die aktuelle optionale Hardware-Caret-Position bereit. ADR 0074 führte den allgemeinen eigenen `TerminalPresentationFrame` ein, und ADR 0075 ermöglichte `TerminalSession`, einen solchen vollständigen Frame-Wert direkt darzustellen.

Zwischen diesen beiden Schichten bleibt eine kleine Ownership-Lücke. Aufrufer, die einen eigenen Frame benötigen, müssen bisher selbst wissen, dass der vollständige Terminal-Presentation-Zustand genau aus `sink.buffer()` und `sink.caretPosition()` besteht, daraus den Frame manuell zusammensetzen und selbst entscheiden, ob eine Referenz oder ein unabhängiger Snapshot gemeint ist.

Diese Duplizierung wird besonders relevant, da transiente Overlays wie Menüs einen vollständigen Anwendungsframe als unveränderliche Basis verwenden können, während der Presentation-Sink seinen veränderlichen Arbeitsbuffer für nachfolgende Widget-Synchronisationsläufe weiterverwendet.

## Entscheidung

`TerminalPresentationSink::captureFrame() const` wird eingeführt.

Die Methode liefert einen `TerminalPresentationFrame` mit:

- einer eigenen Kopie des aktuellen `ScreenBuffer` des Sinks;
- einer Kopie der aktuellen optionalen Hardware-Caret-Position.

Der zurückgegebene Frame ist eine Ownership-Grenze und keine View. Spätere Änderungen am Arbeitsbuffer oder Caret des Sinks beeinflussen einen bereits erfassten Frame nicht; Änderungen am erfassten Frame beeinflussen den Sink ebenfalls nicht.

Der `AmbiguousWidthMode` des Sinks wird bewusst nicht im `TerminalPresentationFrame` gespeichert. Die Behandlung mehrdeutiger Zeichenbreiten ist eine Policy beim Messen und Erzeugen der Terminal-Zellgeometrie. Sobald Zellen und Caret-Koordinaten erfasst sind, repräsentiert der Frame das Presentation-Ergebnis und nicht mehr die Erzeugungs-Policy.

Der Capture-Vorgang kann wegen der vollständigen `ScreenBuffer`-Kopie Speicher allokieren. Allokationsfehler bleiben normale Exceptions. Es wird weder Historie noch ein zusätzlicher Synchronisationsmechanismus eingeführt.

## Konsequenzen

Die Terminal-Presentation-Pipeline erhält damit einen expliziten Wertübergang von der veränderlichen inkrementellen Widget-Darstellung zu einem eigenen vollständigen Frame:

`PresentationCoordinator -> TerminalPresentationSink -> captureFrame() -> TerminalPresentationFrame`

Dieser Frame kann anschließend mit Menüs oder anderen Overlays zusammengesetzt und schließlich über `TerminalSession::present(frame)` ausgegeben werden, ohne dass nachgelagerter Code den Buffer-/Caret-Vertrag erneut manuell zusammensetzen muss.

Die erste Implementierung kopiert bewusst den vollständigen Buffer. Klare Ownership- und Lebensdauersemantik haben aktuell Vorrang vor Optimierung. Ein späterer Optimierungsschritt kann move-/reuse-basierte oder damage-aware Capture-Strategien einführen, solange der beobachtbare Vertrag erhalten bleibt, dass ein erfasster Frame von späteren Sink-Änderungen unabhängig ist.
