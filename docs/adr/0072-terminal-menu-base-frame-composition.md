# ADR 0072: Compose Transient Terminal Menus from an Explicit Base Frame

- Status: Accepted
- Date: 2026-10-02

## Context

The terminal menu pipeline can now reconstruct current semantic interaction state, place every open popup, carry submenu direction, and render the resulting owned frame transactionally into a `ScreenBuffer`.

One lifetime problem remains when this in-place renderer is used across successive frames. A popup that is open in frame N may be closed in frame N+1. The new interaction snapshot correctly contains no popup geometry anymore, so rendering only the current menu state has no knowledge of which cells the old popup covered. Without a freshly reconstructed application frame, those old popup cells could remain visible.

A retained solution would make menu presentation remember previous popup rectangles and actively erase or replay them later. That would introduce hidden frame-history state, duplicate general damage/replay concerns, and couple transient menu lifetime to prior presentation geometry.

## Decision

Add `menu_composition.hpp` with `composeMenuInteractionPresentation()`.

The function receives an explicit immutable base `ScreenBuffer`, the current `MenuBarModel`, `MenuInteractionController`, menu-bar origin, and ambiguous-width policy. It:

1. copies the base buffer into an owned working buffer;
2. invokes the existing `renderMenuInteractionPresentation()` pipeline on that copy;
3. returns the composed buffer when the complete menu interaction is representable and placeable;
4. returns `std::nullopt` on semantic/presentation failure while leaving the base untouched.

The copied buffer's size is the menu viewport. There is therefore no separate viewport argument that can drift from the actual destination surface.

This is a correctness-first full-frame composition model. Closing a popup requires no erase operation: the next composition starts from the same current application/base frame, so cells formerly covered by the popup are naturally restored.

`renderMenuInteractionPresentation()` remains the low-level in-place primitive. Its documentation now explicitly states that it paints only current menu geometry and does not manage historical popup damage. Clients that need repeated transient overlay composition should use the new owned composition boundary or otherwise provide a freshly reconstructed destination frame themselves.

The new layer remains independent from `TerminalSession`, ANSI/VT transport, native terminal handles, widget ownership, and command execution.

## Consequences

The terminal menu stack gains explicit transient-overlay lifetime semantics without introducing retained popup damage state. Frame N+1 is derived from current application content plus current menu state, not from mutation history of frame N.

The initial implementation copies the complete `ScreenBuffer` for each composed frame. This is intentionally accepted in favor of simple invariants and predictable teardown behavior. A later optimization step may use damage regions, double-buffer ownership, or move-based frame exchange while preserving the same observable contract.

Allocation failure during the base copy remains an exception rather than being converted into `std::nullopt`; `std::nullopt` continues to mean that current menu semantics/presentation cannot form a complete valid frame.

---

# ADR 0072: Transiente Terminal-Menüs aus einem expliziten Basisframe zusammensetzen

- Status: Akzeptiert
- Datum: 2026-10-02

## Kontext

Die Terminal-Menü-Pipeline kann inzwischen den aktuellen semantischen Interaktionszustand rekonstruieren, alle geöffneten Popups platzieren, die Submenu-Richtung mitführen und den daraus entstehenden eigenen Frame transaktional in einen `ScreenBuffer` rendern.

Bei aufeinanderfolgenden Frames bleibt jedoch ein Lebensdauerproblem, wenn der In-Place-Renderer direkt verwendet wird. Ein Popup kann in Frame N geöffnet und in Frame N+1 geschlossen sein. Der neue Interaktions-Snapshot enthält dann korrekt keine Popup-Geometrie mehr. Beim Rendern ausschließlich des aktuellen Menüzustands ist deshalb nicht bekannt, welche Zellen das frühere Popup überdeckt hatte. Ohne neu aufgebauten Anwendungsframe könnten alte Popup-Zellen sichtbar bleiben.

Eine zustandsbehaftete Lösung müsste sich frühere Popup-Rechtecke merken und später gezielt löschen oder erneut darstellen. Dadurch entstünde versteckter Frame-Historienzustand, allgemeine Damage-/Replay-Logik würde dupliziert und die Lebensdauer transienter Menüs wäre an frühere Präsentationsgeometrie gekoppelt.

## Entscheidung

`menu_composition.hpp` wird mit `composeMenuInteractionPresentation()` eingeführt.

Die Funktion erhält einen expliziten unveränderlichen Basis-`ScreenBuffer`, das aktuelle `MenuBarModel`, den `MenuInteractionController`, den Ursprung der Menüleiste sowie die Policy für mehrdeutige Zeichenbreiten. Sie:

1. kopiert den Basisbuffer in einen eigenen Arbeitsbuffer;
2. führt darauf die bestehende Pipeline `renderMenuInteractionPresentation()` aus;
3. liefert den zusammengesetzten Buffer zurück, wenn die vollständige Menüinteraktion darstellbar und platzierbar ist;
4. liefert bei semantischem bzw. Presentation-Fehler `std::nullopt`, während der Basisbuffer unverändert bleibt.

Die Größe des kopierten Buffers ist zugleich der Menü-Viewport. Es gibt daher keinen separaten Viewport-Parameter, der von der tatsächlichen Zielfläche abweichen könnte.

Dies ist bewusst ein korrektheitsorientiertes Full-Frame-Kompositionsmodell. Das Schließen eines Popups benötigt keinen Löschvorgang: Die nächste Komposition beginnt erneut beim aktuellen Anwendungs-/Basisframe, wodurch zuvor vom Popup verdeckte Zellen automatisch wiederhergestellt werden.

`renderMenuInteractionPresentation()` bleibt das niedrigere In-Place-Primitiv. Seine Dokumentation weist nun ausdrücklich darauf hin, dass nur aktuelle Menügeometrie gezeichnet und historische Popup-Beschädigung nicht verwaltet wird. Clients mit wiederholter transienter Overlay-Komposition sollen die neue Owned-Kompositionsgrenze verwenden oder selbst einen frisch rekonstruierten Ziel-Frame bereitstellen.

Die neue Schicht bleibt unabhängig von `TerminalSession`, ANSI/VT-Transport, nativen Terminal-Handles, Widget-Ownership und Command-Ausführung.

## Konsequenzen

Der Terminal-Menü-Stack erhält explizite Lebensdauersemantik für transiente Overlays, ohne retained Popup-Damage-State einzuführen. Frame N+1 entsteht aus aktuellem Anwendungsinhalt plus aktuellem Menüzustand und nicht aus der Mutationshistorie von Frame N.

Die erste Implementierung kopiert für jeden zusammengesetzten Frame den vollständigen `ScreenBuffer`. Das wird zugunsten einfacher Invarianten und vorhersehbaren Teardown-Verhaltens bewusst akzeptiert. Ein späterer Optimierungsschritt kann Damage-Regionen, Double-Buffer-Ownership oder move-basierten Frame-Austausch einführen, ohne den beobachtbaren Vertrag zu ändern.

Ein Allokationsfehler beim Kopieren des Basisbuffers bleibt eine Exception und wird nicht in `std::nullopt` übersetzt; `std::nullopt` bedeutet weiterhin, dass aus der aktuellen Menüsemantik/-darstellung kein vollständiger gültiger Frame erzeugt werden kann.
