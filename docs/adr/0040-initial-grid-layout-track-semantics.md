# ADR 0040 – Initial GridLayout track semantics / Initiale GridLayout-Track-Semantik

- **Status:** Accepted
- **Date:** 2026-09-30

## English

### Context

M4 needs a two-dimensional layout primitive before FormLayout and richer form controls. VBox/HBox deliberately postponed a general grid until real controls and both Terminal/Rendered presentation paths existed. That evidence now exists, but a CSS-/WPF-scale grid model with spans, star sizing, alignment and named areas would still freeze too many policies at once.

### Decision

M4 introduces public `sasd::ui::GridLayout : Container` with a deliberately small row-major contract.

- `columnCount` is fixed, positive and defaults to one.
- Visible children fill cells densely in visual adoption order; invisible children collapse and do not reserve a slot.
- Rows are created implicitly as required.
- Each column intrinsic width is the maximum desired width of visible children in that column.
- Each row intrinsic height is the maximum desired height of visible children in that row.
- `columnSpacing` and `rowSpacing` are independent non-negative logical values, inserted only between occupied tracks.
- Measurement forwards the active MeasurementContext to children and gives each child zero minimum plus the parent's maximum as a per-child ceiling.
- During arrangement every visible child stretches to its complete cell.
- Child coordinates remain relative to GridLayout.
- When final width/height is smaller than intrinsic desire, earlier tracks retain their requested extent first and later tracks are clipped to the remaining extent, matching the current VBox/HBox shortage policy.
- Extra final space is intentionally not distributed among tracks in the first contract.
- Arrangement consumes children's last desiredSize() values and does not retain MeasurementContext.
- Accumulation uses saturating Coordinate arithmetic.

The initial contract does not include row/column spans, per-track fixed/auto/star sizing, weights, padding, margins, alignment, baseline rules, RTL reordering, explicit row count, named areas or virtualization.

### Consequences

GridLayout supplies the first backend-neutral multi-axis layout primitive while staying small enough to reason about and test. FormLayout can later build on this evidence instead of inventing its own two-dimensional sizing rules.

Because tracks use intrinsic maxima and do not absorb extra space yet, applications needing expansion can continue to place GridLayout inside an outer container or assign explicit child size constraints. A later pre-1.0 track-sizing policy can add stretch/weights with explicit semantics rather than accidental behavior.

## Deutsch

### Kontext

M4 benötigt vor FormLayout und reichhaltigeren Formular-Controls ein zweidimensionales Layout-Primitiv. VBox/HBox haben ein allgemeines Grid bewusst verschoben, bis reale Controls und Terminal-/Rendered-Presentation praktische Anforderungen liefern. Diese Evidenz ist nun vorhanden; ein Grid im Umfang von CSS/WPF mit Spans, Star-Sizing, Alignment und benannten Bereichen würde aber weiterhin zu viele Regeln auf einmal festschreiben.

### Entscheidung

M4 führt ein öffentliches `sasd::ui::GridLayout : Container` mit bewusst kleinem Row-Major-Vertrag ein.

- `columnCount` ist fest, positiv und standardmäßig eins.
- Sichtbare Kinder füllen Zellen dicht in visueller Adoptionsreihenfolge; unsichtbare Kinder kollabieren und reservieren keinen Slot.
- Zeilen entstehen implizit nach Bedarf.
- Die intrinsische Breite einer Spalte ist die maximale Wunschbreite ihrer sichtbaren Kinder.
- Die intrinsische Höhe einer Zeile ist die maximale Wunschhöhe ihrer sichtbaren Kinder.
- `columnSpacing` und `rowSpacing` sind unabhängige nichtnegative logische Werte und liegen nur zwischen belegten Tracks.
- Measurement reicht den aktiven MeasurementContext an Kinder weiter und gibt jedem Kind Null-Minimum sowie das Parent-Maximum als per-Child-Obergrenze.
- Beim Arrangement streckt sich jedes sichtbare Kind auf seine vollständige Zelle.
- Child-Koordinaten bleiben relativ zu GridLayout.
- Ist finale Breite/Höhe kleiner als der intrinsische Bedarf, behalten frühere Tracks zuerst ihre Wunschgröße und spätere werden auf den verbleibenden Platz gekürzt; dies entspricht der aktuellen VBox/HBox-Policy.
- Zusätzlicher finaler Platz wird im ersten Vertrag bewusst nicht auf Tracks verteilt.
- Arrangement verwendet die letzten `desiredSize()`-Werte der Kinder und speichert keinen MeasurementContext.
- Akkumulation verwendet saturierende Coordinate-Arithmetik.

Der erste Vertrag enthält keine Row-/Column-Spans, per-Track Fixed/Auto/Star-Sizing, Gewichte, Padding, Margins, Alignment, Baseline-Regeln, RTL-Reordering, explizite Row-Anzahl, benannte Bereiche oder Virtualisierung.

### Konsequenzen

GridLayout liefert das erste backendneutrale zweidimensionale Layout-Primitiv und bleibt dabei klein genug, um nachvollziehbar und testbar zu sein. FormLayout kann später auf dieser Erfahrung aufbauen, statt eigene zweidimensionale Sizing-Regeln zu erfinden.

Da Tracks zunächst intrinsische Maxima verwenden und Extra-Platz noch nicht absorbieren, können Anwendungen GridLayout in einem äußeren Container platzieren oder explizite Child-SizeConstraints setzen. Eine spätere Pre-1.0-Track-Sizing-Policy kann Stretch/Gewichte mit expliziter Semantik ergänzen statt zufälliges Verhalten festzuschreiben.
