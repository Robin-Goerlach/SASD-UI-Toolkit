# ADR 0122 – Backend-neutral anchored popup placement and fixed-row geometry

**Status:** Accepted  
**Date:** 2026-10-07

## English

### Context

ADR 0121 completed the Core transaction semantics needed by an open `ComboBox`: committed selection,
transient preview, Enter commit and cancellation. The next popup work needs concrete rectangles, but
putting viewport coordinates into `ComboBox` would violate the existing separation between semantic
Core state and presentation geometry. Implementing separate placement algorithms in Terminal and
Rendered would create the opposite problem: two backends could attach the same semantic drop-down in
different, subtly incompatible ways.

The mature Terminal menu pipeline already has menu-specific placement helpers. Those rules include
menu measurement, submenu left/right relationships and terminal-specific representability, so they are
not rewritten by this decision.

### Decision

Introduce a small generic presentation helper in
`sasd::ui::presentation`:

- `placeAnchoredPopup(...)` chooses a fully visible rectangle above or below an already-arranged
  anchor;
- `PopupVerticalSide` records the chosen side explicitly;
- `fixedPopupRowBounds(...)` maps stable row indices to fixed-height rectangles inside an
  already-established popup *content* rectangle.

The helpers depend only on the existing backend-neutral `Rect`, `Size`, `Point`/`Coordinate`
geometry vocabulary. They do not know about `ComboBox`, Terminal cells, Rendered fonts, SDL, native
windows, Widget ownership or input routing.

#### Anchored placement policy

The first policy is deliberately conservative:

1. anchor, popup and viewport must all have positive area;
2. the complete popup must fit inside the viewport;
3. horizontal origin begins at the anchor's left edge and is clamped only enough to keep the complete
   popup visible;
4. vertically, the preferred side is tried first and the opposite side second;
5. "below" attaches popup top to anchor bottom; "above" attaches popup bottom to anchor top;
6. if neither side can fit the complete popup, placement returns `std::nullopt`.

It does **not** overlap the anchor, clip the popup, reduce its height or silently introduce scrolling.
Those behaviors need explicit later policies because they affect row visibility and interaction.

The viewport is a `Rect`, not only a `Size`, so the same helper remains valid for nested surfaces or
future desktop work areas with non-zero origins.

All edge calculations use `int64_t` before conversion back to `Coordinate`. Placement fails closed
when a returned origin cannot be represented.

#### Fixed-row geometry

`fixedPopupRowBounds(...)` accepts a popup content rectangle, declared row count, requested row index
and backend-selected row height. The complete declared row set must fit inside the supplied content
height. The helper does not know about border/padding: Terminal or Rendered presentation computes its
own inner content rectangle first.

This gives both backends identical row-index geometry while allowing different physical metrics:

- Terminal can use one-cell rows;
- Rendered can use a theme/font-derived logical row height.

No scrolling or virtualization is inferred when rows do not fit; the helper returns `std::nullopt`.

### Consequences

Positive:

- ComboBox popup placement can remain outside semantic Core state;
- Terminal and Rendered can share the same above/below and row-index rules;
- viewport fitting is deterministic and overflow-safe;
- chosen vertical direction is explicit presentation data;
- later pointer hit testing can consume exactly the same final row rectangles used for painting.

Trade-offs:

- popups that cannot fit completely above or below fail closed for now;
- no scrolling, maximum visible row count, monitor/work-area selection or animation policy is included;
- existing Terminal menu placement remains separate because its submenu-specific rules are materially
  different and already stable.

### Deliberately deferred

This ADR does not yet:

- measure ComboBox popup width/height from items;
- add border/padding/theme policy;
- render Terminal or Rendered ComboBox popup rows;
- clip or scroll oversized item lists;
- perform pointer hit testing or outside-click capture;
- alter menu placement code.

Those steps can now build on a common geometry primitive instead of embedding geometry in the control.

---

## Deutsch

### Kontext

ADR 0121 hat die für eine offene `ComboBox` notwendige Core-Transaktion abgeschlossen: committed
Selection, transiente Preview, Enter-Commit und Cancel. Für die nächsten Popup-Schritte brauchen wir
konkrete Rechtecke. Viewport-Koordinaten direkt in `ComboBox` einzubauen würde jedoch die bestehende
Trennung zwischen semantischem Core-Zustand und Presentation-Geometrie verletzen. Zwei getrennte
Placement-Algorithmen für Terminal und Rendered würden dagegen riskieren, dass dasselbe semantische
Drop-down in beiden Backends unterschiedlich funktioniert.

Die ausgereifte Terminal-Menü-Pipeline besitzt bereits menüspezifische Placement-Helfer. Deren Regeln
umfassen Menü-Messung, Links-/Rechts-Beziehungen von Submenus und terminal-spezifische
Darstellbarkeit. Diese bestehende Logik wird durch diese Entscheidung ausdrücklich nicht umgebaut.

### Entscheidung

Es wird ein kleiner generischer Presentation-Helfer unter
`sasd::ui::presentation` eingeführt:

- `placeAnchoredPopup(...)` bestimmt ein vollständig sichtbares Rechteck ober- oder unterhalb eines
  bereits arrangierten Anchors;
- `PopupVerticalSide` hält die gewählte Seite explizit fest;
- `fixedPopupRowBounds(...)` bildet stabile Row-Indizes auf Rechtecke fester Höhe innerhalb eines
  bereits festgelegten Popup-*Content*-Rechtecks ab.

Die Helfer verwenden ausschließlich die vorhandenen backend-neutralen Geometrietypen `Rect`,
`Size`, `Point`/`Coordinate`. Sie kennen weder `ComboBox` noch Terminalzellen, Rendered-Fonts,
SDL, native Fenster, Widget-Ownership oder Input-Routing.

#### Anchored-Placement-Policy

Die erste Policy bleibt bewusst konservativ:

1. Anchor, Popup und Viewport müssen positive Fläche besitzen;
2. das vollständige Popup muss in den Viewport passen;
3. horizontal beginnt das Popup an der linken Anchor-Kante und wird nur so weit verschoben, dass es
   vollständig sichtbar bleibt;
4. vertikal wird zuerst die bevorzugte Seite und danach die Gegenseite versucht;
5. "below" verbindet Popup-Oberkante mit Anchor-Unterkante; "above" verbindet Popup-Unterkante mit
   Anchor-Oberkante;
6. passt das vollständige Popup auf keine Seite, liefert die Funktion `std::nullopt`.

Die Policy überlappt den Anchor nicht, clippt das Popup nicht, reduziert nicht stillschweigend dessen
Höhe und führt kein implizites Scrolling ein. Diese Verhaltensweisen brauchen später eigene explizite
Regeln, weil sie Row-Sichtbarkeit und Interaktion verändern.

Der Viewport ist ein `Rect` statt nur einer `Size`. Damit bleibt derselbe Helfer auch für
verschachtelte Oberflächen oder spätere Desktop-Work-Areas mit einem Ursprung ungleich Null nutzbar.

Alle Kantenberechnungen erfolgen zunächst in `int64_t`; nicht darstellbare Ergebnisursprünge führen zu
einem fail-closed Ergebnis.

#### Fixed-Row-Geometrie

`fixedPopupRowBounds(...)` erhält Content-Rechteck, deklarierte Row-Anzahl, gewünschten Row-Index und
eine vom Backend gewählte Row-Höhe. Die komplette deklarierte Row-Menge muss in die Content-Höhe
passen. Border/Padding kennt dieser Helfer nicht: Terminal- oder Rendered-Presentation berechnet zuerst
sein eigenes inneres Content-Rechteck.

Dadurch verwenden beide Backends dieselbe Row-Index-Geometrie, dürfen aber unterschiedliche physische
Metriken nutzen:

- Terminal kann einzeilige Rows verwenden;
- Rendered kann eine aus Theme/Font abgeleitete logische Row-Höhe verwenden.

Wenn Rows nicht passen, wird weder Scrolling noch Virtualisierung stillschweigend angenommen; die
Funktion liefert `std::nullopt`.

### Konsequenzen

Positiv:

- ComboBox-Popup-Placement bleibt außerhalb des semantischen Core-Zustands;
- Terminal und Rendered können dieselben Above/Below- und Row-Index-Regeln verwenden;
- Viewport-Fitting ist deterministisch und overflow-sicher;
- die gewählte vertikale Richtung ist explizite Presentation-Information;
- späteres Pointer-Hit-Testing kann exakt dieselben finalen Row-Rechtecke konsumieren wie die
  Darstellung.

Abwägungen:

- Popups, die weder vollständig oben noch unten passen, schlagen vorerst fail-closed fehl;
- Scrolling, maximale sichtbare Row-Anzahl, Monitor-/Work-Area-Auswahl und Animation sind nicht
  enthalten;
- bestehendes Terminal-Menü-Placement bleibt separat, weil dessen Submenu-Regeln fachlich anders und
  bereits stabil sind.

### Bewusst vertagt

Diese ADR:

- misst noch nicht Breite/Höhe des ComboBox-Popups aus den Items;
- definiert noch keine Border-/Padding-/Theme-Policy;
- zeichnet noch keine Terminal- oder Rendered-ComboBox-Popup-Rows;
- clippt oder scrollt keine übergroßen Item-Listen;
- implementiert noch kein Pointer-Hit-Testing oder Outside-Click-Capture;
- verändert keinen bestehenden Menü-Placement-Code.

Die folgenden Schritte können nun auf einer gemeinsamen Geometrieprimitive aufbauen, statt Geometrie
in das Control einzubauen.
