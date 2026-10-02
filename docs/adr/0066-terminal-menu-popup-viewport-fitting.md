# ADR 0066: Terminal Menu Popup Viewport Fitting

- Status: Accepted
- Date: 2026-10-02

## Context

ADR 0065 established natural terminal popup placement independently from any concrete viewport. Root popups are placed below top-level menu titles and child popups are placed to the right of their parent rows. That structural geometry is deterministic and matches terminal-cell measurement, but a natural origin may still place part of a popup outside a small terminal surface.

Viewport adaptation is a presentation policy, not menu semantics. It must therefore remain outside `MenuModel` and `MenuInteractionController`, and it should not be mixed into natural parent/child placement. The first policy should also avoid silently inventing clipping, scrolling, or partial popup rendering before those behaviors have explicit contracts.

## Decision

Add `menu_viewport_placement.hpp` at the Terminal presentation boundary with `fitMenuPopupOriginToViewport()`.

The helper receives an owned `MenuPopupPresentationSnapshot`, its already-computed natural `Point`, a viewport `Size`, and the terminal ambiguous-width policy. It reuses `measureMenuPopupPresentation()` so fitting is based on exactly the geometry accepted by the popup renderer.

The initial policy guarantees complete visibility:

- preserve the natural origin when the popup already fits;
- clamp negative origins to zero;
- shift right overflow left to the greatest x origin that still keeps the popup fully visible;
- shift bottom overflow up to the greatest y origin that still keeps the popup fully visible;
- return `std::nullopt` if the popup itself is wider or taller than the viewport;
- return `std::nullopt` for malformed negative viewport dimensions or unrenderable popup content.

A zero-height popup is valid and may be placed at the viewport's bottom edge because it occupies no terminal rows.

The helper accepts a `Size` rather than `ScreenBuffer`. Callers rendering into a buffer pass `buffer.size()`, while the fitting policy itself remains independent from storage, ANSI/VT encoding, and terminal-session I/O.

The policy deliberately does not yet implement directional submenu flipping, clipping, scrolling, or preferred anchor preservation beyond minimal translation. Those concerns require additional explicit contracts and can be layered on top of natural placement later.

## Consequences

Terminal menu placement now has two clean stages: structural natural placement followed by viewport fitting. The semantic model remains backend-neutral, while the Terminal presentation layer can guarantee that a popup either has a fully visible origin or is explicitly reported as not fully placeable.

This conservative first policy may move a submenu left without preserving the visual relationship to its parent when the right edge overflows. A later submenu-aware policy can prefer opening to the left of the parent rectangle; that refinement will not require changing natural placement or menu semantics.

Measurement is repeated intentionally. The current priority is a straightforward and independently testable contract; a later optimization pass may reuse measured popup geometry inside a frame-layout transaction.

---

# ADR 0066: Viewport-Anpassung für Terminal-Menü-Popups

- Status: Akzeptiert
- Datum: 2026-10-02

## Kontext

ADR 0065 hat die natürliche Platzierung von Terminal-Popups unabhängig von einem konkreten Viewport festgelegt. Root-Popups erscheinen unter Top-Level-Menütiteln, Child-Popups rechts neben der zugehörigen Parent-Zeile. Diese strukturelle Geometrie ist deterministisch und verwendet dieselbe Terminal-Zellvermessung wie das Rendering. Trotzdem kann eine natürliche Position bei kleinen Terminalflächen teilweise außerhalb des sichtbaren Bereichs liegen.

Die Anpassung an einen Viewport ist eine Presentation-Policy und keine Menüsemantik. Sie gehört daher weder in `MenuModel` noch in `MenuInteractionController` und soll nicht mit der natürlichen Parent-/Child-Platzierung vermischt werden. Die erste Policy soll außerdem nicht stillschweigend Clipping, Scrolling oder teilweise sichtbare Popups einführen, solange diese Verhaltensweisen keinen eigenen expliziten Vertrag besitzen.

## Entscheidung

An der Terminal-Presentation-Grenze wird `menu_viewport_placement.hpp` mit `fitMenuPopupOriginToViewport()` ergänzt.

Die Funktion erhält einen besitzenden `MenuPopupPresentationSnapshot`, seinen bereits berechneten natürlichen `Point`, eine Viewport-`Size` und die Terminal-Policy für mehrdeutige Zeichenbreiten. Für die Geometrie wird `measureMenuPopupPresentation()` wiederverwendet; Viewport-Fitting basiert damit exakt auf derselben Größe, die auch der Popup-Renderer akzeptiert.

Die erste Policy garantiert vollständige Sichtbarkeit:

- passt das Popup bereits an seiner natürlichen Position, bleibt diese unverändert;
- negative Ursprünge werden auf null verschoben;
- Überlauf nach rechts wird auf den größten x-Ursprung verschoben, bei dem das Popup vollständig sichtbar bleibt;
- Überlauf nach unten wird entsprechend nach oben verschoben;
- ist das Popup selbst breiter oder höher als der Viewport, wird `std::nullopt` zurückgegeben;
- negative Viewport-Dimensionen oder nicht darstellbare Popup-Inhalte führen ebenfalls zu `std::nullopt`.

Ein Popup mit Höhe null ist ein gültiger Presentation-Wert und darf auf der unteren Viewport-Kante liegen, da es keine Terminal-Zeilen belegt.

Die Funktion verwendet eine `Size` statt eines `ScreenBuffer`. Ein Renderer kann einfach `buffer.size()` übergeben; die Fitting-Policy selbst bleibt dadurch unabhängig von Zellenspeicher, ANSI/VT-Encoding und Terminal-Session-I/O.

Bewusst noch nicht Teil dieser Policy sind richtungsabhängiges Spiegeln von Submenus, Clipping, Scrolling oder eine über die minimale Verschiebung hinausgehende Erhaltung des ursprünglichen Ankers. Dafür sollen später eigene explizite Verträge eingeführt werden.

## Konsequenzen

Die Terminal-Menüplatzierung besitzt nun zwei klar getrennte Stufen: strukturelle natürliche Platzierung und anschließendes Viewport-Fitting. Das semantische Modell bleibt backend-neutral, während die Terminal-Presentation-Schicht garantieren kann, dass entweder ein vollständig sichtbarer Popup-Ursprung existiert oder die vollständige Platzierung explizit als unmöglich gemeldet wird.

Bei Überlauf am rechten Rand kann diese konservative erste Policy ein Submenu nach links verschieben, ohne seine visuelle Beziehung zum Parent exakt zu erhalten. Eine spätere submenu-spezifische Policy kann bevorzugt links neben dem Parent öffnen; dafür müssen weder natürliche Platzierung noch Menüsemantik geändert werden.

Die wiederholte Vermessung wird bewusst akzeptiert. Vorrang haben aktuell ein klarer Vertrag und gute Testbarkeit; ein späterer Optimierungsschritt kann vermessene Popup-Geometrie innerhalb einer Frame-Layout-Transaktion wiederverwenden.
