# ADR 0015 – Backend-neutral measurement context for intrinsic widget metrics

**Status:** Accepted  
**Date:** 2026-09-18

## English

### Context

The two-phase layout contract from ADR 0011 requires Widgets to report intrinsic desired sizes before
their parent can arrange them. Some intrinsic sizes are semantic constants, but text and native/rendered
controls depend on presentation-specific metrics.

A `Label` cannot correctly decide its own width from UTF-8 byte count, Unicode scalar count or a
terminal-only width algorithm. A terminal measures in columns/rows, rendered desktop backends use font
metrics in logical units, and native peers may expose platform control metrics.

Putting terminal/font/native measurement directly into `Label` would violate the core/backend
separation. Keeping `Label::measure()` context-free forever would make real automatic layout
impossible.

### Decision

The platform-neutral core introduces `MeasurementContext`.

The contract is deliberately small:

- `MeasurementContext::measureText(std::string_view)` returns backend-neutral logical `Size`;
- `MeasurementContext::revision()` identifies all measurement-affecting state for cache purposes;
- `Widget` keeps its existing context-free `measure(MeasureConstraints)` overload;
- `Widget` additionally exposes `measure(const MeasurementContext&, MeasureConstraints)`;
- context-free and context-aware measurements use separate cache domains;
- context-aware cache identity is the **dynamic MeasurementContext type plus revision**, not the
  address of the context object;
- Widgets never retain a `MeasurementContext*` or reference;
- contexts of the same concrete type and revision promise equivalent measurement semantics;
- a mutable context must change revision whenever measurement-relevant state changes;
- the default context-aware `Widget::onMeasure()` delegates to the existing context-free hook, so
  structural Widgets need no presentation knowledge;
- content Widgets such as `Label` may override the context-aware hook;
- normal Widget `SizeConstraints` and parent `MeasureConstraints` still clamp the intrinsic result
  after the context returns it.

The first backend implementation is `terminal::TerminalMeasurementContext`, which delegates to the
same `terminal::TextMetrics` policy used by terminal rendering. Its logical units are terminal columns
and rows. Its revision is derived from the current East-Asian-Ambiguous width policy, so independent
contexts with equal revision have equal terminal measurement semantics.

### Rationale

Passing a service object into measurement keeps semantic Widgets independent from terminal cells,
fonts, native handles and renderer APIs while still allowing accurate intrinsic sizing.

Not retaining the context object is an important lifetime property. Measurement contexts may be
short-lived stack objects. Caching only static dynamic-type information plus a value revision avoids
dangling references.

Keeping the existing context-free path remains useful for structural Widgets, fixed preferred sizes,
headless tests and incremental migration. It also makes the semantic distinction explicit: a
context-free `Label` has no right to invent a real text metric.

### Alternatives considered

#### Store the active backend/context pointer in every Widget

Rejected. It introduces lifetime coupling, hidden global state and dangling-pointer risk.

#### Make `Label` call terminal TextMetrics directly

Rejected. That would make a platform-neutral semantic Widget depend on one backend's coordinate and
Unicode-width policy.

#### Use UTF-8 length/code-point count as the Label's intrinsic width

Rejected. Those values are not terminal display width and are unrelated to proportional desktop font
metrics.

#### Remove context-free measurement immediately

Deferred. Pre-1.0 API evolution remains possible, but retaining it currently keeps structural Widgets
and existing contracts simple while the multi-backend metric model is still being validated.

#### Cache by MeasurementContext object address

Rejected. A Widget would either retain a potentially dangling identity or risk accidental cache hits
when a later object reuses the same address.

### Consequences

- `Label` can now obtain real intrinsic metrics without knowing the backend;
- terminal layout and terminal rendering share one TextMetrics width policy;
- a later SDL/native measurement context can provide font/control metrics through the same core API;
- layout containers such as `VBox`/`HBox` can propagate one MeasurementContext through child
  measurement;
- context implementations must treat `revision()` as part of their correctness contract;
- the measurement API remains intentionally small until multiple backends demonstrate a need for
  richer font/style/image metric requests.

---

## Deutsch

### Kontext

Der zweiphasige Layout-Vertrag aus ADR 0011 verlangt, dass Widgets ihre intrinsische Wunschgröße
melden, bevor der Parent sie anordnen kann. Manche Größen sind semantische Konstanten; Text und
native/gerenderte Controls hängen jedoch von darstellungsspezifischen Metriken ab.

Ein `Label` kann seine Breite weder aus UTF-8-Bytelänge noch aus Unicode-Codepoint-Anzahl oder einem
Terminal-spezifischen Width-Algorithmus korrekt selbst bestimmen. Das Terminal misst in Spalten und
Zeilen, gerenderte Desktop-Backends verwenden Fontmetriken in logischen Einheiten, native Peers können
Plattform-Control-Metriken liefern.

Terminal-/Font-/Native-Messung direkt in `Label` würde die Core-/Backend-Trennung verletzen.
`Label::measure()` dauerhaft völlig contextlos zu lassen würde dagegen echtes automatisches Layout
verhindern.

### Entscheidung

Der plattformneutrale Core führt `MeasurementContext` ein.

Der Vertrag bleibt bewusst klein:

- `MeasurementContext::measureText(std::string_view)` liefert eine backendneutrale logische `Size`;
- `MeasurementContext::revision()` identifiziert alle messrelevanten Zustände für den Cache;
- `Widget` behält das bestehende contextlose `measure(MeasureConstraints)`;
- zusätzlich existiert `measure(const MeasurementContext&, MeasureConstraints)`;
- contextlose und contextbasierte Messungen besitzen getrennte Cache-Domänen;
- die Cache-Identität ist **dynamischer MeasurementContext-Typ plus Revision**, nicht die Adresse des
  Context-Objekts;
- Widgets speichern weder `MeasurementContext*` noch Referenzen darauf;
- Contexts desselben konkreten Typs mit gleicher Revision versprechen äquivalente Messsemantik;
- ein veränderlicher Context muss bei jeder messrelevanten Zustandsänderung seine Revision ändern;
- das contextbasierte Standard-`Widget::onMeasure()` delegiert an den bisherigen contextlosen Hook,
  sodass strukturelle Widgets kein Backendwissen benötigen;
- Inhalts-Widgets wie `Label` dürfen den contextbasierten Hook überschreiben;
- eigene `SizeConstraints` des Widgets und Parent-`MeasureConstraints` begrenzen das intrinsische
  Context-Ergebnis weiterhin anschließend.

Die erste Backend-Implementierung ist `terminal::TerminalMeasurementContext`. Sie verwendet dieselbe
`terminal::TextMetrics`-Policy wie das Terminal-Rendering. Ihre logischen Einheiten sind
Terminalspalten und -zeilen. Ihre Revision wird direkt aus der aktuellen
East-Asian-Ambiguous-Width-Policy abgeleitet; getrennte Context-Objekte mit gleicher Revision besitzen
damit dieselbe Terminal-Messsemantik.

### Begründung

Ein während der Messung übergebener Service hält semantische Widgets unabhängig von Terminalzellen,
Fonts, nativen Handles und Renderer-APIs und ermöglicht trotzdem korrekte intrinsische Größen.

Dass das Context-Objekt nicht gespeichert wird, ist eine wichtige Lifetime-Eigenschaft.
MeasurementContexts dürfen kurzlebige Stack-Objekte sein. Nur statische dynamische Typinformation plus
ein Revisionswert werden gecacht; dadurch entstehen keine dangling References.

Der bestehende contextlose Pfad bleibt für strukturelle Widgets, feste Preferred Sizes, Headless-Tests
und schrittweise Migration sinnvoll. Er macht außerdem die Semantik klar: Ein contextloses `Label`
darf keine echte Textmetrik erfinden.

### Betrachtete Alternativen

#### Aktives Backend/Context als Pointer in jedem Widget speichern

Verworfen. Das erzeugt Lifetime-Kopplung, versteckten globalen Zustand und Dangling-Pointer-Risiko.

#### `Label` ruft direkt Terminal-TextMetrics auf

Verworfen. Ein plattformneutrales semantisches Widget würde dadurch von Koordinaten- und
Unicode-Width-Regeln genau eines Backends abhängen.

#### UTF-8-Länge/Codepoint-Anzahl als intrinsische Label-Breite

Verworfen. Diese Größen entsprechen weder Terminal-Display-Width noch proportionalen Desktop-Fonts.

#### Contextlose Messung sofort entfernen

Verschoben. Pre-1.0 darf die API weiterentwickelt werden; momentan hält der vorhandene Pfad
strukturelle Widgets und bestehende Verträge einfach, während das Multi-Backend-Metrikmodell praktisch
validiert wird.

#### Cache nach Adresse des MeasurementContext

Verworfen. Das Widget würde entweder eine potenziell dangling Identität speichern oder könnte bei
Wiederverwendung derselben Adresse später einen falschen Cache-Treffer erhalten.

### Konsequenzen

- `Label` kann echte intrinsische Metriken erhalten, ohne das Backend zu kennen;
- Terminal-Layout und Terminal-Rendering teilen sich dieselbe TextMetrics-Width-Policy;
- ein späterer SDL-/Native-MeasurementContext kann Font-/Control-Metriken über dieselbe Core-API
  bereitstellen;
- Layout-Container wie `VBox`/`HBox` können einen MeasurementContext durch die Kindmessung
  weiterreichen;
- Context-Implementierungen müssen `revision()` als Teil ihres Korrektheitsvertrags behandeln;
- die Measurement-API bleibt absichtlich klein, bis mehrere Backends einen belegten Bedarf für
  reichere Font-/Style-/Image-Metrik-Requests zeigen.
