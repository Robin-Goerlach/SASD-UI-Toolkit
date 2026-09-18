# ADR 0017 – Initial VBox/HBox layout semantics

**Status:** Accepted  
**Date:** 2026-09-18

## English

### Context

M2 now has real text metrics through `MeasurementContext` and a conservative presentation-subtree
refresh for geometry changes. This is enough to introduce the first automatic layout containers without
guessing text widths or leaving stale terminal cells after children move.

The first layout implementation should validate the measure/arrange contract across both headless/core
tests and the terminal backend. It should not attempt to solve every future layout policy at once.

### Decision

M2 introduces `VBox` and `HBox` as simple semantic `Container` subclasses.

The initial contract is:

- visual children participate in visual adoption order;
- invisible children do not participate in measurement, arrangement or inter-child spacing;
- `spacing` is a non-negative logical-unit value;
- spacing is inserted only between adjacent visible children;
- `VBox` desired width is the maximum visible child width;
- `VBox` desired height is the sum of visible child heights plus spacing;
- `HBox` desired width is the sum of visible child widths plus spacing;
- `HBox` desired height is the maximum visible child height;
- box measurement forwards the active `MeasurementContext` to children;
- child measurement does not inherit the parent's minimum on each child; it receives a zero minimum
  and the parent's maximum as a per-child ceiling;
- `VBox` stretches children to the full final width while using measured desired height on the main
  axis;
- `HBox` stretches children to the full final height while using measured desired width on the main
  axis;
- child coordinates are relative to the box container;
- when final main-axis space is smaller than measured desire, children are allocated in adoption order;
  earlier visible children keep their requested main-axis size first and later children are clipped to
  the remaining space;
- changing spacing invalidates measurement but does not directly perform layout;
- arrangement expects the normal measure-before-arrange lifecycle; it consumes the children's most
  recent `desiredSize()`;
- arithmetic that accumulates child sizes/spacing saturates at the toolkit Coordinate maximum instead
  of overflowing;
- `VBox` and `HBox` are structural for terminal presentation and do not draw cells themselves.

M2 deliberately does **not** yet define:

- margins or padding;
- per-child alignment;
- flex grow/shrink weights;
- equal-size policies;
- wrapping;
- baseline alignment;
- reverse direction;
- RTL-sensitive ordering;
- per-child minimum allocation/fairness when the final main axis is too small;
- layout animations.

### Rationale

These rules provide a small, deterministic vertical/horizontal layout model that can immediately lay out
real `Label` widgets using terminal metrics and can later reuse desktop font metrics through the same
`MeasurementContext`.

Stretching the cross axis gives the box ownership of one axis while keeping the main axis driven by each
child's intrinsic desired size. This is simple enough to reason about before alignment and flex policies
exist.

Adoption-order clipping is intentionally basic. It provides deterministic behavior under insufficient
space without prematurely inventing a flex/shrink algorithm. Later policy can evolve under the pre-1.0
compatibility rules once Button/TextField and desktop prototypes provide better evidence.

### Alternatives considered

#### Absolute positioning only for M2

Rejected. It would not validate the measure/arrange and MeasurementContext contracts through a real
container.

#### Implement a general FlexBox/Grid system immediately

Deferred. It would introduce too many policy decisions before the first controls and second backend
exercise them.

#### Invisible children keep their layout slot

Rejected for the initial box contract. Hiding a child should collapse its slot in the first user-facing
layout model; visibility already invalidates ancestor measurement.

#### Give every child the parent's minimum constraints

Rejected. Two children would each be told to satisfy the full parent minimum, which does not represent
how a shared box distributes space.

### Consequences

- M2 now has the first automatic layout path from `Label` text metrics through `VBox`/`HBox` to
  terminal cells;
- visibility changes can collapse/reflow box layout;
- geometry reflow is safe because ADR 0016 forces appropriate subtree presentation refresh;
- later Button/TextField controls can participate through the same Widget measurement contract;
- richer alignment/flex/padding policies remain explicit future work rather than hidden assumptions.

---

## Deutsch

### Kontext

M2 besitzt inzwischen echte Textmetriken über `MeasurementContext` und einen konservativen
Presentation-Subtree-Refresh für Geometrieänderungen. Damit können die ersten automatischen
Layout-Container eingeführt werden, ohne Textbreiten zu raten oder nach dem Verschieben von Kindern alte
Terminalzellen stehenzulassen.

Die erste Layout-Implementierung soll den Measure-/Arrange-Vertrag sowohl in Headless-/Core-Tests als
auch im Terminal-Backend praktisch validieren. Sie soll nicht gleichzeitig jede zukünftige
Layout-Policy lösen.

### Entscheidung

M2 führt `VBox` und `HBox` als einfache semantische `Container`-Unterklassen ein.

Der erste Vertrag lautet:

- visuelle Kinder nehmen in visueller Adoptionsreihenfolge teil;
- unsichtbare Kinder nehmen weder an Measurement noch Arrangement oder Inter-Child-Spacing teil;
- `spacing` ist ein nichtnegativer Wert in logischen Einheiten;
- Spacing liegt ausschließlich zwischen benachbarten sichtbaren Kindern;
- die Wunschbreite einer `VBox` ist die maximale sichtbare Kindbreite;
- die Wunschhöhe einer `VBox` ist Summe der sichtbaren Kindhöhen plus Spacing;
- die Wunschbreite einer `HBox` ist Summe der sichtbaren Kindbreiten plus Spacing;
- die Wunschhöhe einer `HBox` ist die maximale sichtbare Kindhöhe;
- Box-Measurement reicht den aktiven `MeasurementContext` an Kinder weiter;
- ein Kind erhält nicht das Parent-Minimum als eigenes Minimum, sondern Null-Minimum und das
  Parent-Maximum als per-Child-Obergrenze;
- `VBox` streckt Kinder auf die volle finale Breite und verwendet auf der Hauptachse ihre gemessene
  Wunschhöhe;
- `HBox` streckt Kinder auf die volle finale Höhe und verwendet auf der Hauptachse ihre gemessene
  Wunschbreite;
- Child-Koordinaten sind relativ zum Box-Container;
- reicht der finale Platz auf der Hauptachse nicht aus, erfolgt die Zuteilung in Adoptionsreihenfolge:
  frühere sichtbare Kinder behalten zuerst ihre Wunschgröße, spätere werden auf den Rest gekürzt;
- eine Spacing-Änderung invalidiert Measurement, führt aber nicht selbst Layout aus;
- Arrangement setzt den normalen Measure-vor-Arrange-Lebenszyklus voraus und verwendet das jeweils
  letzte `desiredSize()` der Kinder;
- akkumulierte Child-/Spacing-Arithmetik saturiert am maximalen `Coordinate`-Wert statt zu
  überlaufen;
- `VBox` und `HBox` sind für Terminal-Presentation strukturell und zeichnen selbst keine Zellen.

M2 legt bewusst **noch nicht** fest:

- Margin oder Padding;
- Child-Alignment;
- Flex-Grow-/Shrink-Gewichte;
- Equal-Size-Regeln;
- Wrapping;
- Baseline-Alignment;
- umgekehrte Richtung;
- RTL-abhängige Reihenfolge;
- faire/minimale Verteilung bei zu kleinem finalen Platz;
- Layout-Animationen.

### Begründung

Diese Regeln liefern ein kleines deterministisches vertikales/horizontales Layout, das reale
`Label`-Widgets bereits mit Terminalmetriken anordnet und später über denselben
`MeasurementContext` Desktop-Fontmetriken verwenden kann.

Stretch auf der Querachse gibt der Box die Kontrolle über eine Achse, während die Hauptachse weiterhin
von der intrinsischen Wunschgröße des Kindes bestimmt wird. Das ist vor Alignment- und Flex-Policies
leicht nachvollziehbar.

Das Clipping in Adoptionsreihenfolge ist bewusst einfach. Es definiert bei Platzmangel ein
deterministisches Verhalten, ohne vorschnell einen Flex-/Shrink-Algorithmus zu erfinden. Unter den
Pre-1.0-Regeln kann diese Policy später anhand von Button/TextField und Desktop-Prototypen weiter
entwickelt werden.

### Betrachtete Alternativen

#### In M2 nur absolute Positionierung

Verworfen. Dadurch würden Measure/Arrange und MeasurementContext nicht durch einen realen Container
validiert.

#### Sofort ein allgemeines FlexBox-/Grid-System

Verschoben. Das würde zu viele Policy-Entscheidungen einführen, bevor erste Controls und ein zweites
Backend sie praktisch prüfen.

#### Unsichtbare Kinder behalten ihren Layoutplatz

Für den ersten Box-Vertrag verworfen. Ein ausgeblendetes Kind soll im ersten benutzbaren Layoutmodell
seinen Slot zusammenklappen; Visibility invalidiert bereits Parent-Measurement.

#### Jedes Kind erhält das Parent-Minimum

Verworfen. Zwei Kinder würden jeweils aufgefordert, das vollständige Parent-Minimum zu erfüllen, obwohl
sie sich denselben Box-Raum teilen.

### Konsequenzen

- M2 besitzt jetzt den ersten automatischen Pfad von `Label`-Textmetriken über `VBox`/`HBox` bis
  zu Terminalzellen;
- Visibility-Änderungen können Box-Layout kollabieren und neu fließen lassen;
- Reflow-Geometrie ist durch ADR 0016 presentationseitig sicher;
- spätere Button-/TextField-Controls können denselben Widget-Measurement-Vertrag verwenden;
- reichere Alignment-/Flex-/Padding-Policies bleiben ausdrücklich zukünftige Arbeit.
