# ADR 0073: Compose Terminal Menu Cursor Metadata with the Overlay Frame

- Status: Accepted
- Date: 2026-10-02

## Context

ADR 0072 introduced correctness-first composition of transient terminal menus from an explicit immutable base `ScreenBuffer`. That solves cell lifetime: when a popup closes, the next composed frame starts from current application cells and old popup chrome disappears naturally.

Terminal presentation also has frame metadata that is not stored in `ScreenBuffer`: the hardware-caret request produced by focused `TextField` presentation. A menu can become active while semantic widget focus remains unchanged. If menu composition carries only cells, a caller may accidentally keep presenting the underlying TextField caret while the user is navigating the menu bar or popup.

Changing `FocusManager` or `TextField` merely to hide that caret would mix transient menu interaction with semantic application focus and would require an unnecessary focus restoration cycle when the menu closes.

## Decision

Extend `menu_composition.hpp` with an owned `MenuComposedPresentationFrame` containing:

- the composed `ScreenBuffer`;
- an optional hardware-caret position.

Add `composeMenuInteractionFrame()` with an explicit base caret argument. It uses the existing cell-composition pipeline and applies one presentation-only caret policy:

- when `MenuInteractionController::isActive()` is false, propagate the base caret unchanged;
- when menu interaction is active, suppress the caret in the composed frame;
- when the controller becomes inactive again, a new composition from the same base metadata naturally restores the base caret.

The policy does not mutate semantic focus, `TextField`, `TerminalPresentationSink`, or `MenuInteractionController`. It is derived entirely from current state.

The existing `composeMenuInteractionPresentation()` API remains available for callers that only need cell composition. It delegates to the new metadata-aware path with no base caret so transient cell lifetime has one implementation.

Composition failure remains fail-closed. If the menu frame cannot be built or rendered completely, `composeMenuInteractionFrame()` returns `std::nullopt`; no partially composed cell/caret result is published.

This layer still does not call `TerminalSession::present()`. Transport remains a separate responsibility and can consume the returned buffer/caret pair later.

## Consequences

Application focus and terminal cursor visibility are now explicitly separate concerns. A focused TextField can remain the semantic focus owner while active menu interaction temporarily suppresses its visible hardware caret.

The returned value is closer to the actual information required by `TerminalSession::present(buffer, caret)`, but composition and transport remain decoupled and independently testable.

The first implementation continues to copy the complete base `ScreenBuffer`. Cursor metadata adds no retained history and does not change the correctness-first optimization policy from ADR 0072.

---

# ADR 0073: Terminal-Menü-Komposition um Cursor-Metadaten des Overlay-Frames erweitern

- Status: Akzeptiert
- Datum: 2026-10-02

## Kontext

ADR 0072 führte die korrektheitsorientierte Komposition transienter Terminal-Menüs aus einem expliziten unveränderlichen Basis-`ScreenBuffer` ein. Damit ist die Lebensdauer der Zellen gelöst: Schließt ein Popup, beginnt der nächste zusammengesetzte Frame erneut mit den aktuellen Anwendungszellen und alte Popup-Darstellung verschwindet automatisch.

Zur Terminal-Präsentation gehören jedoch auch Frame-Metadaten, die nicht im `ScreenBuffer` gespeichert werden: die Hardware-Caret-Anforderung eines fokussierten `TextField`. Ein Menü kann aktiv werden, während der semantische Widget-Fokus unverändert bleibt. Würde die Menü-Komposition nur Zellen transportieren, könnte ein Aufrufer versehentlich weiterhin den Caret des darunterliegenden Textfelds anzeigen, während der Benutzer die Menüleiste oder ein Popup bedient.

`FocusManager` oder `TextField` nur zum Verbergen dieses Carets zu verändern, würde transiente Menüinteraktion mit dem semantischen Anwendungsfokus vermischen und beim Schließen des Menüs einen unnötigen Fokus-Wiederherstellungszyklus erfordern.

## Entscheidung

`menu_composition.hpp` wird um einen eigenen `MenuComposedPresentationFrame` erweitert. Dieser enthält:

- den zusammengesetzten `ScreenBuffer`;
- eine optionale Hardware-Caret-Position.

Zusätzlich wird `composeMenuInteractionFrame()` mit einem expliziten Basis-Caret eingeführt. Die Funktion verwendet die bestehende Zell-Kompositionspipeline und wendet genau eine Presentation-spezifische Caret-Policy an:

- ist `MenuInteractionController::isActive()` falsch, wird der Basis-Caret unverändert weitergegeben;
- ist die Menüinteraktion aktiv, wird der Caret im zusammengesetzten Frame unterdrückt;
- wird der Controller wieder inaktiv, stellt eine neue Komposition aus denselben Basis-Metadaten den Basis-Caret automatisch wieder her.

Die Policy verändert weder den semantischen Fokus noch `TextField`, `TerminalPresentationSink` oder `MenuInteractionController`. Sie wird ausschließlich aus dem aktuellen Zustand abgeleitet.

Die bestehende API `composeMenuInteractionPresentation()` bleibt für Aufrufer erhalten, die nur die Zell-Komposition benötigen. Sie delegiert mit fehlendem Basis-Caret an den neuen Metadaten-Pfad, sodass es nur eine Implementierung der transienten Zell-Lebensdauer gibt.

Fehler bleiben fail-closed. Kann der Menüframe nicht vollständig aufgebaut oder gerendert werden, liefert `composeMenuInteractionFrame()` `std::nullopt`; es wird kein teilweise zusammengesetztes Zell-/Caret-Ergebnis veröffentlicht.

Diese Schicht ruft weiterhin nicht `TerminalSession::present()` auf. Der Transport bleibt eine getrennte Verantwortung und kann später das zurückgegebene Buffer-/Caret-Paar konsumieren.

## Konsequenzen

Anwendungsfokus und Sichtbarkeit des Terminal-Cursors sind nun ausdrücklich getrennte Belange. Ein fokussiertes Textfeld kann semantischer Fokusbesitzer bleiben, während aktive Menüinteraktion seinen sichtbaren Hardware-Caret vorübergehend unterdrückt.

Der zurückgegebene Wert entspricht damit stärker den Informationen, die `TerminalSession::present(buffer, caret)` tatsächlich benötigt; Komposition und Transport bleiben jedoch entkoppelt und unabhängig testbar.

Die erste Implementierung kopiert weiterhin den vollständigen Basis-`ScreenBuffer`. Die Cursor-Metadaten führen keine Historie ein und ändern die korrektheitsorientierte Optimierungsstrategie aus ADR 0072 nicht.
