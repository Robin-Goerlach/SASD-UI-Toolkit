# ADR 0065: Natural Terminal Menu-Popup Placement

- Status: Accepted
- Date: 2026-10-02

## Context

The Terminal backend now owns complete menu-presentation frames containing a persistent top-level menu bar and zero or more positioned popup snapshots. The frame renderer intentionally does not decide where popup layers should appear. Callers therefore still need deterministic placement rules that are consistent with the same terminal-cell measurements used for rendering.

Popup placement is a presentation concern. It must not leak terminal-cell geometry into `MenuModel` or `MenuInteractionController`, and it should remain independent from ANSI/VT output and native terminal sessions. At the same time, viewport fitting is a distinct policy problem: a structurally natural popup position may extend beyond a small terminal, where later code may choose to shift, flip or clip it.

## Decision

Add `menu_placement.hpp` at the Terminal presentation boundary with two pure placement helpers:

- `naturalMenuBarPopupOrigin()` computes the origin of a root popup below one top-level menu title. The popup starts at the left edge of that title's padded terminal span and one row below the menu bar.
- `naturalSubmenuPopupOrigin()` computes the origin of a child popup immediately to the right of the measured parent popup and vertically aligned with the submenu item row that opened it.

Both helpers use the existing terminal menu measurement contract before returning geometry. An invalid menu index, a non-submenu anchor, unsupported menu text, saturated measurement, or coordinate overflow returns `std::nullopt` instead of producing approximate or wrapped geometry.

All coordinate arithmetic is widened to `int64_t` and range-checked before conversion back to the public signed `Coordinate` type.

The functions intentionally compute only **natural placement**. They receive no `ScreenBuffer` or viewport size and do not clamp, flip, scroll or clip popup rectangles. A later viewport-fitting layer can transform these natural origins according to an explicit policy while preserving the structural parent/child geometry established here.

## Consequences

Terminal popup geometry now has a deterministic source that matches the same Unicode width and padding rules as rendering. Root and nested popup placement can therefore be tested headlessly without coupling geometry to device I/O or semantic menu state.

Separating natural placement from viewport fitting avoids prematurely baking small-screen behavior into the semantic model. It also leaves room for different future policies, such as opening a submenu to the left when the right side is unavailable, without changing the basic menu interaction or snapshot contracts.

The implementation intentionally repeats a small amount of measurement work. Correctness and a simple contract are preferred until profiling justifies caching or a richer per-frame layout object.

---

# ADR 0065: Natürliche Platzierung von Terminal-Menü-Popups

- Status: Akzeptiert
- Datum: 2026-10-02

## Kontext

Das Terminal-Backend besitzt inzwischen vollständige Menü-Presentation-Frames mit einer permanenten Top-Level-Menüleiste und null oder mehreren positionierten Popup-Snapshots. Der Frame-Renderer entscheidet bewusst nicht selbst, wo Popup-Ebenen erscheinen sollen. Aufrufer benötigen daher noch deterministische Platzierungsregeln, die zu denselben Terminal-Zellmessungen passen, die auch beim Rendering verwendet werden.

Popup-Platzierung ist eine Presentation-Verantwortung. Terminal-Zellgeometrie darf weder in `MenuModel` noch in `MenuInteractionController` gelangen und soll unabhängig von ANSI/VT-Ausgabe und nativen Terminal-Sessions bleiben. Gleichzeitig ist Viewport-Anpassung ein eigenes Policy-Problem: Eine strukturell natürliche Popup-Position kann bei einem kleinen Terminal außerhalb der sichtbaren Fläche liegen; spätere Logik kann dann explizit verschieben, spiegeln oder clippen.

## Entscheidung

An der Terminal-Presentation-Grenze wird `menu_placement.hpp` mit zwei reinen Platzierungsfunktionen ergänzt:

- `naturalMenuBarPopupOrigin()` berechnet den Ursprung eines Root-Popups unter einem Top-Level-Menütitel. Das Popup beginnt an der linken Kante des gepaddeten Terminal-Bereichs dieses Titels und eine Zeile unter der Menüleiste.
- `naturalSubmenuPopupOrigin()` berechnet den Ursprung eines Child-Popups unmittelbar rechts neben dem vermessenen Parent-Popup und vertikal ausgerichtet an der Submenu-Zeile, die es geöffnet hat.

Beide Funktionen verwenden vor der Rückgabe die bestehende Terminal-Menüvermessung. Ein ungültiger Menüindex, ein Nicht-Submenu-Anker, nicht unterstützter Menütext, saturierte Messung oder Koordinatenüberlauf führt zu `std::nullopt`, statt angenäherte oder übergelaufene Geometrie zu erzeugen.

Sämtliche Koordinatenarithmetik wird zunächst auf `int64_t` erweitert und erst nach einer Bereichsprüfung wieder in den öffentlichen vorzeichenbehafteten Typ `Coordinate` zurückgeführt.

Die Funktionen berechnen bewusst nur die **natürliche Platzierung**. Sie erhalten weder `ScreenBuffer` noch Viewport-Größe und klemmen, spiegeln, scrollen oder clippen Popup-Rechtecke nicht. Eine spätere Viewport-Fitting-Schicht kann diese natürlichen Ursprünge anhand einer expliziten Policy transformieren und dabei die hier festgelegte strukturelle Parent-/Child-Geometrie erhalten.

## Konsequenzen

Die Terminal-Popup-Geometrie besitzt nun eine deterministische Quelle, die exakt dieselben Unicode-Breiten- und Padding-Regeln wie das Rendering verwendet. Die Platzierung von Root- und verschachtelten Popups kann damit headless getestet werden, ohne Geometrie an Geräte-I/O oder semantischen Menüzustand zu koppeln.

Die Trennung von natürlicher Platzierung und Viewport-Fitting verhindert, dass Verhalten für kleine Bildschirme vorschnell in das semantische Modell eingebaut wird. Sie lässt außerdem Raum für spätere Policies, etwa ein Submenu nach links zu öffnen, wenn rechts kein Platz vorhanden ist, ohne die grundlegenden Menüinteraktions- oder Snapshot-Verträge zu verändern.

Die Implementierung akzeptiert zunächst etwas wiederholte Vermessung. Korrektheit und ein einfacher Vertrag haben Vorrang, bis Profiling Caching oder ein umfangreicheres Frame-Layout-Objekt rechtfertigt.
