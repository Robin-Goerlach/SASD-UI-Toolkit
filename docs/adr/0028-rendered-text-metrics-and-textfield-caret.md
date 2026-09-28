# ADR 0028 – Rendered text metrics and TextField caret/viewport contract

**Status:** Accepted  
**Date:** 2026-09-28

## English

### Context

ADR 0015 introduced the backend-neutral `MeasurementContext` so semantic widgets can obtain real
intrinsic sizes without knowing terminal cells, fonts or native controls. That contract is sufficient
for labels and basic layout, but M3 exposes a stricter requirement for an editable rendered
`TextField`: presentation must know the font line height and the horizontal position of the insertion
caret at a Unicode-scalar boundary.

Those positions cannot be derived correctly from UTF-8 byte count, scalar count or terminal cell
width. A rendered font may be proportional and shaping can change advances through kerning,
ligatures or script-specific rules. At the same time, placing SDL, FreeType, HarfBuzz or native font
objects into `TextField` would violate the established Core/backend boundary.

The first `RenderedPresentationSink` therefore kept visible TextFields `deferred` rather than
acknowledging an incomplete drawing without reliable viewport/caret geometry.

### Decision

The separately linkable `SASD::UI::Rendered` layer introduces
`rendered::RenderedMeasurementContext : MeasurementContext`.

It keeps the general Core measurement contract and adds only the rendered text information that the
current TextField presentation demonstrably needs:

- `lineHeight()` returns the positive logical line height used by the active rendered font metrics;
- `textAdvanceToScalar(text, index)` returns the logical horizontal advance from the complete text
  run's origin to one Unicode-scalar boundary;
- scalar boundary zero resolves to advance zero;
- available boundary advances are non-negative and monotonically non-decreasing in the initial
  left-to-right model;
- `std::nullopt` is an ordinary capability result when the current shaping/metric provider cannot
  express a boundary faithfully with this initial model.

The same concrete `RenderedMeasurementContext` is intended to be used for
`Widget::measure(context, ...)` and by `RenderedPresentationSink`. The sink observes the context
without owning it; semantic Widgets still retain no presentation-service pointer or native font
resource.

`RenderedMeasurementContext` also supplies the current rendered Button/TextField intrinsic chrome
measurement. A TextField reserves one logical unit of border on each side and one additional logical
unit for the end caret so focus does not immediately change its natural layout requirement.

When synchronizing a visible TextField with metrics, `RenderedPresentationSink`:

1. resolves all coordinates and metric queries before appending any drawing command;
2. defers transactionally if a required boundary is unavailable, invalid or unrepresentable;
3. chooses the horizontal viewport start only on a Unicode-scalar boundary;
4. keeps the **complete UTF-8 text run** in `DrawTextCommand`, shifts its logical origin by the
   viewport start advance, and clips it to the field interior;
5. draws the insertion caret last as an existing filled-rectangle primitive.

Keeping the complete run is deliberate. A concrete font renderer can shape the same full text context
rather than reshaping a sliced suffix whose kerning/ligature behavior might differ.

The constructor of `RenderedPresentationSink` without a metric context remains valid for
Window/Label/Button consumers. A visible TextField remains `deferred` through that constructor,
which preserves the conservative pre-metric behavior.

The initial contract is intentionally single-line and left-to-right. A provider that cannot map
complex visual ordering into monotonic scalar-boundary advances returns `std::nullopt`; the field
remains pending. A later bidi/shaping milestone may introduce a richer visual-caret map instead of
forcing that complexity into the first M3 slice.

### Rationale

This keeps three responsibilities separate:

- semantic editing state stays in `TextField`;
- font/shaping knowledge stays in the rendered metric provider;
- drawing remains an ordered backend-neutral `DisplayList` consumed later by SDL3 or another device
  adapter.

It also preserves a key M1/M2 rule: incomplete presentation is not silently acknowledged.

Using the same metric object for layout and caret/viewport calculations reduces the risk that a field
is measured with one policy but presented with another.

### Alternatives considered

#### Derive caret positions from UTF-8 bytes or Unicode-scalar count

Rejected. Neither represents proportional font advances or shaping.

#### Measure independent text prefixes/substrings in the sink

Rejected. Re-shaping substrings can produce different kerning or ligature results from the complete
run.

#### Put font/shaping state into TextField

Rejected. It would couple the semantic Core to one rendered/native implementation and introduce
resource-lifetime concerns into Widgets.

#### Add caret-specific methods to the base MeasurementContext

Rejected for now. Terminal and future native-peer contexts should not be forced to implement a
rendered-font contract that only the current M3 path needs.

#### Couple the contract directly to SDL3 or a font library

Rejected. SDL3 remains an optional adapter below `SASD::UI::Rendered`, and the eventual font/shaping
implementation must remain replaceable.

#### Add a dedicated DisplayList caret primitive immediately

Rejected. The current insertion caret is representable as a filled logical rectangle. The command
vocabulary should only grow when existing primitives cannot represent a demonstrated requirement.

### Consequences

- rendered TextField presentation can now be tested deterministically without a desktop window;
- a concrete desktop font adapter must provide metrics consistent with the font/shaping policy used
  to execute `DrawTextCommand`;
- unsupported complex caret ordering remains explicit through `std::nullopt` and `deferred`;
- the current implementation may query multiple scalar boundaries per update; this prioritizes
  correctness and a clean contract over premature caching;
- future shaping/run caches can optimize those queries behind the same boundary;
- SDL3, FreeType, HarfBuzz and native font handles remain absent from Core and generic DisplayList
  APIs.

---

## Deutsch

### Kontext

ADR 0015 führte den backendneutralen `MeasurementContext` ein, damit semantische Widgets echte
intrinsische Größen erhalten können, ohne Terminalzellen, Fonts oder native Controls zu kennen. Für
Labels und grundlegendes Layout genügt dieser Vertrag. M3 zeigt beim editierbaren gerenderten
`TextField` jedoch einen zusätzlichen Bedarf: Die Darstellung muss die Font-Zeilenhöhe und die
horizontale Position des Einfüge-Carets an einer Unicode-Scalar-Grenze kennen.

Diese Positionen lassen sich weder aus UTF-8-Bytelänge noch aus Scalar-Anzahl oder Terminal-Zellbreite
korrekt ableiten. Ein gerenderter Font kann proportional sein; Shaping kann Advances durch Kerning,
Ligaturen oder schriftspezifische Regeln verändern. SDL-, FreeType-, HarfBuzz- oder native
Fontobjekte im `TextField` würden dagegen die bestehende Core-/Backend-Grenze verletzen.

Der erste `RenderedPresentationSink` ließ sichtbare TextFields deshalb bewusst `deferred`, statt
eine unvollständige Darstellung ohne verlässliche Viewport-/Caret-Geometrie als synchronisiert zu
bestätigen.

### Entscheidung

Die separat linkbare Schicht `SASD::UI::Rendered` führt
`rendered::RenderedMeasurementContext : MeasurementContext` ein.

Der allgemeine Core-Messvertrag bleibt erhalten. Ergänzt werden nur die gerenderten Textinformationen,
die die heutige TextField-Darstellung konkret benötigt:

- `lineHeight()` liefert die positive logische Zeilenhöhe der aktiven gerenderten Fontmetriken;
- `textAdvanceToScalar(text, index)` liefert den logischen horizontalen Advance vom Ursprung des
  vollständigen Textlaufs bis zu einer Unicode-Scalar-Grenze;
- Scalar-Grenze null liegt bei Advance null;
- verfügbare Grenz-Advances sind im ersten Links-nach-rechts-Modell nichtnegativ und monoton
  nichtfallend;
- `std::nullopt` ist ein normaler Capability-Zustand, wenn ein Shaping-/Metrik-Provider eine Grenze
  mit diesem ersten Modell nicht korrekt ausdrücken kann.

Derselbe konkrete `RenderedMeasurementContext` soll sowohl für
`Widget::measure(context, ...)` als auch für den `RenderedPresentationSink` verwendet werden. Der
Sink beobachtet den Context, besitzt ihn aber nicht; semantische Widgets speichern weiterhin weder
Presentation-Service-Pointer noch native Fontressourcen.

`RenderedMeasurementContext` liefert außerdem die aktuellen intrinsischen Chrome-Maße für gerenderte
Buttons und TextFields. Ein TextField reserviert auf jeder Seite eine logische Border-Einheit und
zusätzlich eine logische Einheit für das End-Caret. Dadurch verändert der reine Fokuswechsel nicht
sofort den natürlichen Layoutbedarf.

Bei der Synchronisierung eines sichtbaren TextFields mit Metriken führt der
`RenderedPresentationSink` folgende Schritte aus:

1. alle Koordinaten und Metrikabfragen werden vor dem ersten neuen Zeichenbefehl geprüft;
2. fehlt eine benötigte Grenze oder ist sie ungültig/nicht darstellbar, bleibt das Update
   transaktional `deferred`;
3. der horizontale Viewport beginnt nur an einer Unicode-Scalar-Grenze;
4. der **vollständige UTF-8-Textlauf** bleibt im `DrawTextCommand`; sein logischer Ursprung wird um
   den Viewport-Start-Advance verschoben und anschließend auf das Feldinnere geclippt;
5. das Einfüge-Caret wird zuletzt mit dem bereits vorhandenen Fill-Rectangle-Primitiv gezeichnet.

Der vollständige Textlauf ist Absicht. Ein konkreter Fontrenderer kann denselben vollständigen Kontext
shapen, statt einen abgeschnittenen Suffix neu zu formen, dessen Kerning-/Ligaturverhalten abweichen
könnte.

Der Konstruktor des `RenderedPresentationSink` ohne Metrik-Context bleibt für
Window-/Label-/Button-Anwender gültig. Ein sichtbares TextField bleibt über diesen Konstruktor
`deferred`; damit bleibt das konservative Verhalten der ersten M3-Stufe erhalten.

Der erste Vertrag ist bewusst einzeilig und Links-nach-rechts. Kann ein Provider komplexe visuelle
Reihenfolge nicht in monotone Scalar-Grenz-Advances abbilden, liefert er `std::nullopt`; das Feld
bleibt pending. Ein späterer Bidi-/Shaping-Meilenstein kann dafür eine reichere Visual-Caret-Map
einführen, statt diese Komplexität in den ersten M3-Schnitt zu zwingen.

### Begründung

Damit bleiben drei Verantwortlichkeiten getrennt:

- der semantische Editierzustand liegt im `TextField`;
- Font-/Shaping-Wissen liegt im Rendered-Metrik-Provider;
- Zeichnung bleibt eine geordnete backendneutrale `DisplayList`, die später ein SDL3- oder anderer
  Device-Adapter ausführt.

Außerdem bleibt eine wichtige M1/M2-Regel erhalten: Unvollständige Presentation wird nicht
stillschweigend bestätigt.

Die gemeinsame Verwendung desselben Metrikobjekts für Layout und Caret/Viewport verringert das Risiko,
dass ein Feld mit einer Policy gemessen und mit einer anderen dargestellt wird.

### Betrachtete Alternativen

#### Caret-Positionen aus UTF-8-Bytes oder Unicode-Scalar-Anzahl ableiten

Verworfen. Beides bildet weder proportionale Font-Advances noch Shaping ab.

#### Einzelne Textpräfixe/-suffixe im Sink separat messen

Verworfen. Erneutes Shaping von Teilstrings kann andere Kerning-/Ligaturergebnisse als der
vollständige Textlauf erzeugen.

#### Font-/Shaping-Zustand im TextField speichern

Verworfen. Dadurch würde der semantische Core an eine Rendered-/Native-Implementierung gekoppelt und
Ressourcen-Lifetime in Widgets hineingezogen.

#### Caret-spezifische Methoden in den Basis-MeasurementContext aufnehmen

Vorerst verworfen. Terminal- und spätere Native-Peer-Contexts sollen keinen gerenderten Fontvertrag
implementieren müssen, den aktuell nur der M3-Pfad benötigt.

#### Vertrag direkt an SDL3 oder eine Fontbibliothek koppeln

Verworfen. SDL3 bleibt ein optionaler Adapter unterhalb von `SASD::UI::Rendered`; auch die spätere
Font-/Shaping-Implementierung soll austauschbar bleiben.

#### Sofort ein eigenes DisplayList-Caret-Primitiv einführen

Verworfen. Das heutige Einfüge-Caret lässt sich als gefülltes logisches Rechteck darstellen. Das
Befehlsvokabular soll erst wachsen, wenn vorhandene Primitive einen belegten Bedarf nicht ausdrücken
können.

### Konsequenzen

- gerenderte TextField-Presentation ist jetzt ohne Desktopfenster deterministisch testbar;
- ein konkreter Desktop-Fontadapter muss Metriken liefern, die zur Font-/Shaping-Policy beim Ausführen
  von `DrawTextCommand` passen;
- komplexe, noch nicht darstellbare Caret-Reihenfolge bleibt über `std::nullopt` und `deferred`
  explizit;
- die aktuelle Implementierung darf pro Update mehrere Scalar-Grenzen abfragen; Korrektheit und
  sauberer Vertrag haben zunächst Vorrang vor Caching;
- spätere Shaping-/Run-Caches können diese Abfragen hinter derselben Grenze optimieren;
- SDL3-, FreeType-, HarfBuzz- und native Fonthandles bleiben außerhalb von Core und generischer
  DisplayList-API.
