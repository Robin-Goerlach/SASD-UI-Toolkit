# ADR 0041 – Initial FormLayout semantics / Initiale FormLayout-Semantik

- **Status:** Accepted
- **Date:** 2026-10-01

## English

### Context

GridLayout provides a general intrinsic row-major two-dimensional primitive, but ordinary data-entry forms have a recurring asymmetry: labels should line up at one stable intrinsic width while editors should consume additional horizontal space. Implementing that behavior ad hoc in every example would duplicate layout policy, while immediately adding star/weight sizing to GridLayout would enlarge its public contract before enough evidence exists.

### Decision

M4 introduces `sasd::ui::FormLayout : Container` as a specialized two-column form policy.

- Visual children are interpreted as stable label/field pairs in visual adoption order. Pairing is structural and does not change when one member becomes invisible.
- `emplaceRow<T>(label, args...)` is the preferred convenience API. It creates a `Label` followed by one Widget-derived field and returns non-owning typed references to both.
- An odd final visual child is a label-only row, keeping direct Container adoption deterministic rather than rejecting a partially constructed custom form.
- A row participates while at least one of its two cells is visible. A row with both cells hidden collapses and contributes no row spacing.
- Label-column intrinsic width is the maximum desired width of visible label cells.
- Field-column intrinsic width is the maximum desired width of visible field cells.
- Row intrinsic height is the maximum desired height of the visible cells in that row.
- `columnSpacing` exists only when the form has at least one visible label and one visible field globally. `rowSpacing` is inserted only between active rows.
- Measurement forwards the active MeasurementContext to visible children and gives each child zero minimum plus the parent maximum as a per-child ceiling.
- During arrangement the label column keeps its intrinsic width subject to clipping. The field column receives all remaining final width after the label column and gap, so fields expand naturally in wider forms.
- Vertical shortage follows the existing deterministic Box/Grid rule: earlier active rows keep their intrinsic height first and later rows are clipped to the remaining extent.
- Child coordinates remain relative to FormLayout and arrangement consumes previously measured desiredSize values without retaining MeasurementContext.

The first contract deliberately does not define per-row label alignment, baseline alignment, help/error text cells, required markers, label placement above fields, row/column spans, margins/padding, or general Grid star/weight sizing.

### Rationale

FormLayout is intentionally a semantic layout policy rather than a new rendering primitive. Terminal, Rendered, and future native peers continue to present the child widgets normally; only their geometry changes. Keeping field expansion local to FormLayout also provides practical evidence before deciding whether GridLayout itself needs public weighted tracks.

Structural pairing instead of visible-child packing matters for dynamic forms: hiding a label or editor must not shift all following children into the opposite column. The convenience `emplaceRow()` reduces accidental ordering mistakes while preserving Container's lower-level adoption model for advanced users.

### Consequences

FormLayout makes aligned resizable forms possible without exposing backend details or prematurely enlarging GridLayout. Its first implementation shares the existing Grid layout translation unit because both policies use the same saturating coordinate and constraint rules; the public types remain separate.

A future validation/binding slice can attach behavior to the field Widgets without changing this geometry contract. If real forms demonstrate a need for richer label placement or weighted tracks, those can be added explicitly under the pre-1.0 compatibility policy.

## Deutsch

### Kontext

GridLayout liefert ein allgemeines intrinsisches zweidimensionales Row-Major-Primitiv. Normale Eingabeformulare besitzen jedoch eine wiederkehrende Asymmetrie: Beschriftungen sollen mit einer stabilen intrinsischen Breite ausgerichtet sein, während Eingabefelder zusätzlichen horizontalen Platz nutzen sollen. Diese Regel in jedem Beispiel einzeln zu implementieren würde Layout-Policy duplizieren; sofort Star-/Weight-Sizing in GridLayout einzubauen würde dessen öffentlichen Vertrag vergrößern, bevor genügend praktische Evidenz vorliegt.

### Entscheidung

M4 führt `sasd::ui::FormLayout : Container` als spezialisierte zweispaltige Formular-Policy ein.

- Visuelle Kinder werden als stabile Label-/Field-Paare in visueller Adoptionsreihenfolge interpretiert. Die Paarung ist strukturell und ändert sich nicht, wenn ein Mitglied unsichtbar wird.
- `emplaceRow<T>(label, args...)` ist die bevorzugte Convenience-API. Sie erzeugt ein `Label` gefolgt von einem Widget-abgeleiteten Field und liefert nichtbesitzende typisierte Referenzen auf beide zurück.
- Ein ungerades letztes visuelles Kind bildet eine reine Label-Zeile; direkte Container-Adoption bleibt damit deterministisch statt ein teilweise aufgebautes Custom-Formular abzulehnen.
- Eine Zeile nimmt teil, solange mindestens eine ihrer beiden Zellen sichtbar ist. Sind beide unsichtbar, kollabiert die Zeile einschließlich Row-Spacing.
- Die intrinsische Label-Spaltenbreite ist die maximale Wunschbreite sichtbarer Label-Zellen.
- Die intrinsische Field-Spaltenbreite ist die maximale Wunschbreite sichtbarer Field-Zellen.
- Die intrinsische Zeilenhöhe ist die maximale Wunschhöhe der sichtbaren Zellen dieser Zeile.
- `columnSpacing` existiert nur, wenn global mindestens ein sichtbares Label und ein sichtbares Field vorhanden sind. `rowSpacing` liegt ausschließlich zwischen aktiven Zeilen.
- Measurement reicht den aktiven MeasurementContext an sichtbare Kinder weiter und gibt jedem Kind Null-Minimum sowie das Parent-Maximum als per-Child-Obergrenze.
- Beim Arrangement behält die Label-Spalte ihre intrinsische Breite, soweit Platz vorhanden ist. Die Field-Spalte erhält den gesamten verbleibenden finalen Platz nach Label-Spalte und Gap, sodass Eingabefelder in breiteren Formularen natürlich expandieren.
- Vertikaler Platzmangel folgt der bestehenden deterministischen Box-/Grid-Regel: frühere aktive Zeilen behalten zuerst ihre intrinsische Höhe, spätere werden auf den verbleibenden Platz gekürzt.
- Child-Koordinaten bleiben relativ zu FormLayout; Arrangement verwendet vorher gemessene desiredSize-Werte und speichert keinen MeasurementContext.

Der erste Vertrag definiert bewusst noch kein per-Row Label-Alignment, Baseline-Alignment, Help-/Error-Text-Zellen, Required-Marker, Labels oberhalb von Feldern, Row-/Column-Spans, Margins/Padding oder allgemeines Grid-Star-/Weight-Sizing.

### Begründung

FormLayout ist bewusst eine semantische Layout-Policy und kein neues Rendering-Primitiv. Terminal-, Rendered- und spätere Native-Peers präsentieren die Child-Widgets unverändert; lediglich ihre Geometrie ändert sich. Die Field-Expansion lokal in FormLayout zu halten liefert außerdem praktische Evidenz, bevor entschieden wird, ob GridLayout selbst öffentliche gewichtete Tracks benötigt.

Die strukturelle Paarung statt Packing nur sichtbarer Kinder ist für dynamische Formulare wichtig: Das Ausblenden eines Labels oder Editors darf nicht alle folgenden Kinder in die jeweils andere Spalte verschieben. Die Convenience-API `emplaceRow()` reduziert versehentliche Reihenfolgefehler und bewahrt gleichzeitig das niedrigere Container-Adoptionsmodell für fortgeschrittene Nutzer.

### Konsequenzen

FormLayout ermöglicht ausgerichtete, resizable Formulare ohne Backenddetails und ohne GridLayout vorschnell zu vergrößern. Die erste Implementierung teilt sich die bestehende Grid-Layout-Übersetzungseinheit, weil beide Policies dieselben saturierenden Koordinaten- und Constraint-Regeln verwenden; die öffentlichen Typen bleiben getrennt.

Ein späterer Validation-/Binding-Slice kann Verhalten an die Field-Widgets koppeln, ohne diesen Geometrievertrag zu ändern. Wenn reale Formulare reichhaltigere Label-Platzierung oder gewichtete Tracks benötigen, kann das unter der Pre-1.0-Kompatibilität explizit ergänzt werden.
