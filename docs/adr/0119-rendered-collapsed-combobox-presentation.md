# ADR 0119 – Rendered collapsed ComboBox presentation

**Status:** Accepted  
**Date:** 2026-10-07

## English

### Context

ADR 0117 introduced the backend-neutral non-editable `ComboBox` selection contract, and ADR 0118
implemented the first collapsed Terminal presentation. The next vertical slice is the equivalent
collapsed control in the backend-neutral Rendered layer.

Rendered presentation differs from Terminal presentation in one important way: the control is not a
fixed cell string. Font metrics, logical border thickness and the right-side drop affordance all need
to come from the same rendered measurement policy that produced the Widget's intrinsic size. The
slice must therefore remain independent from SDL/native types while still keeping layout and drawing
geometrically coherent.

### Decision

`RenderedMeasurementContext` overrides `measureComboBox(std::string_view)`. The intrinsic collapsed
size is composed from:

1. the measured item text;
2. one gap between text and the indicator lane, at least one logical unit and at least the configured
   control border thickness;
3. a stable indicator lane whose width is the maximum of line height and the measured width of the
   ASCII marker `v`;
4. the normal control border on all sides.

The content height is the maximum of item-text height, indicator-text height and positive line height.
All additions use the existing saturating arithmetic. Core continues to measure the empty baseline and
every owned item through this hook, so the widest item determines intrinsic width and a pure selection
change cannot resize the ComboBox.

`RenderedPresentationSink` requires a `RenderedMeasurementContext` for a visible ComboBox. This is
intentional: guessing line height or indicator geometry in the metrics-free compatibility sink would
break the measurement/presentation contract. When metrics are absent, synchronization returns
`deferred` before changing the DisplayList.

The collapsed presentation is:

- one opaque repaint of the complete arranged bounds;
- the normal rendered control border;
- selected text left-aligned inside the bordered content and clipped before the indicator lane;
- a right-anchored indicator lane containing the ASCII `v`, centered from the same text metrics.

No-selection state leaves the text area empty but still renders the border and drop indicator. The
indicator lane is anchored to the arranged right edge, so changing between short and long selections
never moves the affordance. If a parent layout gives the control more than its intrinsic width, the
extra space stays between selected text and the right-side indicator rather than changing semantics.

Focus and disabled appearance reuse the existing `controlTextStyle()` overlay:

- focus adds `TextStyle::inverse`;
- disabled adds `TextStyle::dim`;
- user-provided `ComboBox::textStyle()` remains unchanged.

All widened coordinate calculations and the inset operation are completed before the first DisplayList
mutation. An unrepresentable geometry therefore leaves the previous frame intact and the ComboBox
pending. After preflight, the complete control is erased before the new selection is drawn, preventing
stale pixels when a long selection is replaced by a shorter one.

### Deliberately deferred

This slice still does **not** introduce:

- open/closed ComboBox state;
- popup list presentation or placement;
- pointer opening/capture outside the collapsed bounds;
- preview selection versus committed selection;
- Escape rollback or type-ahead;
- SDL-specific ComboBox logic or native peers.

Those behaviors need their own semantic and presentation contracts. In particular, popup geometry must
not be hidden inside the collapsed renderer or leaked into Core merely to make the first graphic
ComboBox look complete.

### Consequences

Positive:

- Core, Terminal and Rendered now share one useful collapsed ComboBox selection/measurement contract.
- Rendered layout and drawing derive from the same font/theme service.
- Selection changes preserve geometry and repaint transactionally.
- No SDL, window-system or native peer type enters Core or generic Rendered APIs.

Trade-offs:

- The initial drop affordance is the text marker `v`, not a vector chevron. DisplayList currently has
  rectangle and text primitives only; using the measured marker avoids inventing a new graphics
  primitive solely for one control.
- A metrics-free Rendered sink cannot render ComboBox, just as current CheckBox/RadioButton/TextField
  paths already require rendered metrics for coherent geometry.

---

## Deutsch

### Kontext

ADR 0117 hat den backend-neutralen Auswahlvertrag der nicht editierbaren `ComboBox` eingeführt, ADR
0118 anschließend die erste geschlossene Terminaldarstellung. Der nächste vertikale Slice ist das
entsprechende geschlossene Control in der backend-neutralen Rendered-Schicht.

Rendered Presentation unterscheidet sich dabei wesentlich von der Terminaldarstellung: Das Control ist
keine feste Zellzeichenfolge. Fontmetriken, logische Border-Breite und der rechte Drop-Indikator müssen
aus derselben Rendered-Messpolicy stammen, die auch die intrinsische Widget-Größe erzeugt hat. Der Slice
muss deshalb SDL-/Native-unabhängig bleiben und trotzdem Layout und Zeichnung geometrisch konsistent
halten.

### Entscheidung

`RenderedMeasurementContext` überschreibt `measureComboBox(std::string_view)`. Die intrinsische
Größe der geschlossenen ComboBox setzt sich zusammen aus:

1. dem gemessenen Item-Text;
2. einem Abstand zwischen Text und Indikatorbereich, mindestens eine logische Einheit und mindestens
   so breit wie die konfigurierte Control-Border;
3. einem stabilen Indikatorbereich, dessen Breite das Maximum aus Line Height und der gemessenen Breite
   des ASCII-Markers `v` ist;
4. der normalen Control-Border auf allen Seiten.

Die Content-Höhe ist das Maximum aus Item-Texthöhe, Indikator-Texthöhe und positiver Line Height. Alle
Additionen verwenden die vorhandene saturierende Arithmetik. Der Core misst weiterhin die leere Basis
und jedes eigene Item über diesen Hook. Damit bestimmt das breiteste Item die intrinsische Breite, und
ein reiner Selection-Wechsel kann die ComboBox nicht vergrößern oder verkleinern.

`RenderedPresentationSink` benötigt für eine sichtbare ComboBox einen
`RenderedMeasurementContext`. Das ist bewusst so: Line Height oder Indikatorgeometrie im
metrics-freien Kompatibilitätspfad zu erraten, würde den Measurement-/Presentation-Vertrag brechen.
Fehlen Metriken, liefert die Synchronisation deshalb vor jeder DisplayList-Änderung `deferred`.

Die geschlossene Darstellung besteht aus:

- einem vollständigen opaken Repaint der arrangierten Bounds;
- der normalen Rendered-Control-Border;
- linksbündigem Selection-Text innerhalb des Border-Contents, vor dem Indikatorbereich geclippt;
- einem rechts verankerten Indikatorbereich mit dem ASCII-`v`, anhand derselben Textmetriken zentriert.

Ohne Selection bleibt der Textbereich leer; Border und Drop-Indikator werden trotzdem dargestellt.
Der Indikatorbereich bleibt an der rechten arrangierten Kante. Ein Wechsel zwischen kurzem und langem
Selection-Text verschiebt ihn daher nicht. Gibt ein Parent-Layout mehr als die intrinsische Breite,
bleibt dieser zusätzliche Platz zwischen Text und rechtem Indikator, ohne die Semantik zu verändern.

Fokus und Disabled-Zustand nutzen den vorhandenen `controlTextStyle()`-Overlay-Vertrag:

- Fokus ergänzt `TextStyle::inverse`;
- Disabled ergänzt `TextStyle::dim`;
- der vom Anwender gesetzte `ComboBox::textStyle()` bleibt unverändert.

Alle verbreiterten Koordinatenberechnungen und das Inset werden vor der ersten DisplayList-Mutation
abgeschlossen. Nicht darstellbare Geometrie lässt damit den letzten Frame unangetastet und die ComboBox
pending. Nach erfolgreichem Preflight wird das komplette Control gelöscht, bevor die neue Auswahl
gemalt wird. Beim Wechsel von langem zu kurzem Text können so keine alten Pixel stehenbleiben.

### Bewusst vertagt

Auch dieser Slice führt **nicht** ein:

- Open/Closed-State der ComboBox;
- Popup-Listendarstellung oder Popup-Platzierung;
- Pointer-Opening/Capture außerhalb der geschlossenen Bounds;
- Preview-Selection gegenüber committed Selection;
- Escape-Rollback oder Type-ahead;
- SDL-spezifische ComboBox-Logik oder Native Peers.

Diese Funktionen benötigen eigene semantische und Presentation-Verträge. Insbesondere darf
Popup-Geometrie nicht im collapsed Renderer versteckt oder nur für eine optisch vollständige erste
Grafik-ComboBox in den Core gezogen werden.

### Konsequenzen

Positiv:

- Core, Terminal und Rendered teilen nun einen brauchbaren collapsed ComboBox-Selection-/Measurement-
  Vertrag.
- Rendered Layout und Zeichnung stammen aus demselben Font-/Theme-Dienst.
- Selection-Wechsel behalten ihre Geometrie und repainten transaktional.
- Weder SDL-/Window-System- noch Native-Peer-Typen gelangen in Core oder generische Rendered-APIs.

Abwägungen:

- Der erste Drop-Indikator ist der Textmarker `v` und noch kein Vektor-Chevron. DisplayList besitzt
  derzeit nur Rechteck- und Textprimitiven; der gemessene Marker vermeidet eine neue Grafikprimitive
  nur für ein einzelnes Control.
- Ein metrics-freier Rendered-Sink kann die ComboBox nicht darstellen, genauso wie die vorhandenen
  CheckBox-/RadioButton-/TextField-Pfade für konsistente Geometrie bereits Rendered-Metriken benötigen.
