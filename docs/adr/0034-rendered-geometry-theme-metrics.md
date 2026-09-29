# ADR 0034 – Rendered geometry theme metrics before a full theme system

**Status:** Accepted  
**Date:** 2026-09-29

## English

### Context

The M3 rendered path already has observable control geometry: Button/TextField borders, the pressed
Button caption offset, and the TextField insertion-caret width. Until now those values were duplicated
as literal one-unit assumptions across `RenderedMeasurementContext`, `RenderedPresentationSink`,
TextField viewport geometry and click-to-caret hit testing.

The defaults were consistent by convention, but there was no architectural guarantee that intrinsic
measurement and presentation would stay aligned once a real desktop theme needed different values.
Putting such geometry into semantic `Button` or `TextField` would also violate the Core/backend
boundary.

At the same time, the project deliberately does not yet need a complete theme engine with colors,
brushes, fonts, inheritance, platform appearance objects or style cascading.

### Decision

`SASD::UI::Rendered` introduces the small value type `RenderedThemeMetrics`.

Its first contract contains only geometry already demonstrated by current controls:

- `control_border_thickness`;
- `button_pressed_offset`;
- `text_field_caret_width`.

The default values preserve the existing one-logical-unit M3 appearance.

`RenderedMeasurementContext::themeMetrics()` supplies this policy alongside rendered font/shaping
metrics. The default implementation returns the standard M3 metrics, so existing concrete adapters
remain source-compatible. A provider whose theme metrics can change at runtime must include that
change in `revision()` because control intrinsic sizes depend on the policy.

The same metric snapshot is consumed by:

- `RenderedMeasurementContext::measureButton()` and `measureTextField()`;
- `RenderedPresentationSink` for Button/TextField chrome;
- the shared TextField viewport/caret geometry;
- `RenderedTextFieldHitTest`, through that same viewport calculation.

Values are normalized at the Rendered boundary: negative border/pressed offsets become zero and a
non-positive caret width becomes one. This prevents malformed custom providers from generating
negative rectangles or non-positive drawing primitives.

The pressed offset remains presentation-only and therefore does not contribute to Button intrinsic
measurement.

### Rationale

This is the smallest coherent abstraction that removes duplicated geometry without prematurely
creating a general styling subsystem.

Keeping the policy in the Rendered layer preserves semantic widgets and generic Core measurement from
desktop-rendering details. Supplying it through the same `RenderedMeasurementContext` used for font
metrics also gives layout and presentation one policy identity and one revision mechanism.

A single snapshot per presentation/hit-test operation avoids internally mixing old/new values if a
future platform-backed provider observes live theme state.

### Alternatives considered

#### Keep literal constants until a complete theme system exists

Rejected. The duplication already crosses measurement, drawing and pointer mapping, so divergence is
a current architectural risk rather than a hypothetical future feature.

#### Put border/caret metrics into Button and TextField

Rejected. Those values describe one rendered presentation strategy, not semantic control state.

#### Add theme geometry to the base MeasurementContext

Rejected. Terminal and future native contexts should not be forced to expose rendered-control
geometry.

#### Introduce colors, fonts, brushes and cascading now

Rejected. No demonstrated M3 requirement justifies that much API surface yet.

### Consequences

- default M3 appearance is unchanged;
- non-default rendered chrome can be tested without SDL/native types;
- measurement, painting and click-to-caret share the same border/caret policy;
- RenderedMeasurementContext implementations that make theme metrics mutable must update revision();
- a later full theme subsystem can grow from this narrow geometry contract without moving theme state
  into Core widgets.

---

## Deutsch

### Kontext

Der gerenderte M3-Pfad besitzt bereits sichtbare Control-Geometrie: Borders für Button/TextField, den
Pressed-Offset der Button-Beschriftung und die Breite des TextField-Einfüge-Carets. Bisher waren diese
Werte als fest verdrahtete Einheiten über `RenderedMeasurementContext`,
`RenderedPresentationSink`, TextField-Viewport und Click-to-Caret-Hit-Test verteilt.

Die Defaultwerte waren nur per Konvention konsistent. Sobald ein echtes Desktop-Theme andere Werte
benötigt, gab es keine architektonische Garantie mehr, dass intrinsisches Measurement und
Presentation zusammenpassen. Diese Geometrie in semantische `Button`- oder `TextField`-Objekte zu
legen, würde zugleich die Core-/Backend-Grenze verletzen.

Ein vollständiges Theme-System mit Farben, Brushes, Fonts, Vererbung, Plattformobjekten oder
Style-Cascade wird dafür noch nicht benötigt.

### Entscheidung

`SASD::UI::Rendered` führt den kleinen Werttyp `RenderedThemeMetrics` ein.

Der erste Vertrag enthält ausschließlich bereits belegte geometrische Größen:

- `control_border_thickness`;
- `button_pressed_offset`;
- `text_field_caret_width`.

Die Defaultwerte erhalten die bisherige M3-Darstellung mit einer logischen Einheit.

`RenderedMeasurementContext::themeMetrics()` liefert diese Policy gemeinsam mit den gerenderten
Font-/Shaping-Metriken. Die Defaultimplementierung liefert die bisherigen Standardwerte, sodass
bestehende konkrete Adapter source-kompatibel bleiben. Ändert ein Provider Theme-Metriken zur
Laufzeit, muss sich auch `revision()` ändern, weil die intrinsischen Control-Größen davon abhängen.

Derselbe Metrik-Snapshot wird verwendet durch:

- `RenderedMeasurementContext::measureButton()` und `measureTextField()`;
- `RenderedPresentationSink` für Button-/TextField-Chrome;
- die gemeinsame TextField-Viewport-/Caret-Geometrie;
- `RenderedTextFieldHitTest` über dieselbe Viewport-Berechnung.

An der Rendered-Grenze werden Werte defensiv normalisiert: negative Border-/Pressed-Werte werden null,
eine nichtpositive Caret-Breite wird eins. Dadurch erzeugen fehlerhafte Custom-Provider keine
negativen Rechtecke oder unzulässigen Zeichenprimitive.

Der Pressed-Offset bleibt reine Presentation und beeinflusst deshalb die intrinsische Button-Größe
nicht.

### Begründung

Das ist die kleinste kohärente Abstraktion, die duplizierte Geometrie entfernt, ohne vorzeitig ein
allgemeines Styling-System zu bauen.

Die Policy bleibt in der Rendered-Schicht; semantische Widgets und der generische Core kennen keine
Desktop-Renderdetails. Weil dieselbe `RenderedMeasurementContext`-Instanz bereits Fontmetriken
bereitstellt, erhalten Layout und Presentation zugleich eine gemeinsame Policy-Identität und einen
gemeinsamen Revision-Mechanismus.

Ein einzelner Snapshot pro Presentation-/Hit-Test-Operation verhindert außerdem, dass ein späterer
plattformgestützter Provider innerhalb eines Updates alte und neue Live-Theme-Werte mischt.

### Betrachtete Alternativen

#### Konstanten bis zu einem vollständigen Theme-System beibehalten

Verworfen. Die Duplizierung betrifft bereits heute Measurement, Drawing und Pointer-Mapping und ist
damit ein aktuelles Architekturrisiko.

#### Border-/Caret-Metriken in Button und TextField speichern

Verworfen. Diese Werte beschreiben eine gerenderte Darstellung, nicht den semantischen Control-Zustand.

#### Theme-Geometrie in den Basis-MeasurementContext aufnehmen

Verworfen. Terminal- und spätere Native-Contexts sollen keinen Rendered-spezifischen Geometrievertrag
implementieren müssen.

#### Jetzt bereits Farben, Fonts, Brushes und Cascade einführen

Verworfen. Dafür gibt es in M3 noch keinen belegten Bedarf.

### Konsequenzen

- die bisherige M3-Defaultdarstellung bleibt unverändert;
- nicht-standardmäßige Rendered-Chrome kann ohne SDL-/Native-Typen getestet werden;
- Measurement, Painting und Click-to-Caret teilen dieselbe Border-/Caret-Policy;
- veränderliche Theme-Metriken müssen in der Revision des RenderedMeasurementContext erscheinen;
- ein späteres vollständiges Theme-System kann aus diesem kleinen Geometrievertrag wachsen, ohne
  Theme-Zustand in Core-Widgets zu verschieben.
