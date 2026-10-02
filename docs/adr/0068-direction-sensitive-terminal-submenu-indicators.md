# ADR 0068: Direction-Sensitive Terminal Submenu Indicators

- Status: Accepted
- Date: 2026-10-02

## Context

ADR 0067 made child-submenu placement direction explicit through `SubmenuPopupSide`. The terminal popup renderer, however, still draws the same `>` marker for every submenu row. When a child popup is viewport-fitted to the left of its parent, that marker no longer reflects the actual presentation direction.

The semantic menu model must not learn about terminal opening direction. Direction is chosen only after terminal measurement and viewport policy have run, so it belongs to presentation. At the same time, the existing popup renderer already owns text, shortcut alignment, selection styling, clipping and wide-cell correctness. Duplicating that renderer merely to replace one marker would create two implementations of the same row layout contract.

## Decision

Add `menu_directional_presentation.hpp` at the Terminal presentation boundary.

Introduce `ActiveSubmenuPresentationDirection`, which carries the one submenu item index in a popup level that currently owns an open child and the already-decided `SubmenuPopupSide`. A menu popup level can have at most one direct child open at once, so the direction does not need to be copied into every item snapshot.

Add `renderDirectionalMenuPopupPresentation()`. The function validates that the supplied index exists and identifies a submenu before modifying `ScreenBuffer`. It also preflights the popup with the existing terminal measurement contract. It then delegates all ordinary row rendering to `renderMenuPopupPresentation()` and replaces only the reserved submenu-marker cell:

- `>` for a child opening to the right;
- `<` for a child opening to the left.

Both markers occupy one narrow ASCII cell, so popup measurement does not change. Marker styling is recomputed through the same row-style helper as the ordinary renderer, preserving inverse selection and disabled styling.

The renderer consumes the explicit side returned by viewport placement rather than inferring direction from coordinates. This keeps direction stable even if later placement policies introduce gaps or controlled overlap.

The ordinary non-directional popup renderer remains available and continues to use `>` as its default submenu marker. Directional chrome is therefore opt-in until frame composition is extended to carry the active-child direction automatically.

## Consequences

Terminal popup chrome can now represent the real left/right opening direction without leaking terminal geometry into Core menu semantics. The implementation reuses the existing popup renderer rather than duplicating layout and Unicode behavior.

Invalid or stale direction descriptors fail closed before any buffer mutation. Unsupported popup text continues to follow the existing transactional preflight contract.

The next integration step can attach the active-child direction to composed menu-frame presentation so callers do not need to invoke the directional renderer manually.

---

# ADR 0068: Richtungsabhängige Terminal-Submenu-Indikatoren

- Status: Akzeptiert
- Datum: 2026-10-02

## Kontext

ADR 0067 macht die Platzierungsrichtung eines Child-Submenus mit `SubmenuPopupSide` explizit. Der Terminal-Popup-Renderer zeichnet jedoch weiterhin für jede Submenu-Zeile denselben Marker `>`. Wird ein Child-Popup wegen der Viewport-Grenzen links neben seinem Parent geöffnet, beschreibt dieser Marker die tatsächliche Darstellungsrichtung nicht mehr korrekt.

Das semantische Menümodell darf die terminal-spezifische Öffnungsrichtung nicht kennen. Die Richtung entsteht erst nach Terminal-Vermessung und Viewport-Policy und gehört deshalb in die Presentation-Schicht. Gleichzeitig besitzt der vorhandene Popup-Renderer bereits die Verträge für Text, Shortcut-Ausrichtung, Selection-Styling, Clipping und Wide-Cell-Korrektheit. Nur wegen eines anderen Markers einen zweiten vollständigen Renderer zu bauen, würde denselben Zeilenlayout-Vertrag doppelt implementieren.

## Entscheidung

An der Terminal-Presentation-Grenze wird `menu_directional_presentation.hpp` ergänzt.

`ActiveSubmenuPresentationDirection` speichert den einen Submenu-Item-Index einer Popup-Ebene, der aktuell ein geöffnetes Child besitzt, sowie die bereits entschiedene `SubmenuPopupSide`. Pro Popup-Ebene kann höchstens ein direktes Child gleichzeitig geöffnet sein; die Richtung muss daher nicht in jedes Item-Snapshot kopiert werden.

`renderDirectionalMenuPopupPresentation()` prüft vor jeder Änderung des `ScreenBuffer`, dass der Index existiert und tatsächlich eine Submenu-Zeile bezeichnet. Zusätzlich wird der Popup-Snapshot mit dem bestehenden Terminal-Messvertrag vorgeprüft. Danach delegiert die Funktion das normale Zeilenrendering vollständig an `renderMenuPopupPresentation()` und ersetzt ausschließlich die bereits reservierte Marker-Zelle:

- `>` für ein nach rechts öffnendes Child;
- `<` für ein nach links öffnendes Child.

Beide Marker belegen genau eine schmale ASCII-Zelle, deshalb ändert sich die Popup-Vermessung nicht. Das Marker-Styling wird mit demselben Row-Style-Helper wie im normalen Renderer berechnet, sodass inverse Selection und Disabled-Darstellung erhalten bleiben.

Der Renderer verwendet die explizite Seite der Viewport-Platzierung und leitet die Richtung nicht aus Koordinaten ab. Damit bleibt die Information auch dann eindeutig, wenn spätere Policies Abstände oder kontrollierte Überlappung einführen.

Der normale nicht-richtungsabhängige Popup-Renderer bleibt bestehen und verwendet weiterhin `>` als Default-Marker. Richtungsabhängiges Chrome ist damit zunächst opt-in, bis die Frame-Komposition die aktive Child-Richtung automatisch mitführt.

## Konsequenzen

Terminal-Popup-Chrome kann jetzt die tatsächliche Links-/Rechts-Öffnungsrichtung darstellen, ohne Terminal-Geometrie in die Core-Menüsemantik zu tragen. Die Implementierung verwendet den bestehenden Popup-Renderer weiter, statt Layout- und Unicode-Verhalten zu duplizieren.

Ungültige oder veraltete Richtungsdeskriptoren werden fail-closed vor jeder Buffer-Mutation abgelehnt. Nicht unterstützter Popup-Text folgt weiterhin dem bestehenden transaktionalen Preflight-Vertrag.

Als nächster Integrationsschritt kann die aktive Child-Richtung an die zusammengesetzte Menüframe-Presentation angehängt werden, sodass Aufrufer den richtungsabhängigen Renderer nicht manuell auswählen müssen.
