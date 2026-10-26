# ADR 0184: M8 Widget SizeConstraints Compound Property Coverage

- Status: Accepted
- Date: 2026-10-25

## English

### Context

[ADR 0177](0177-m8-serializable-ui-description-state-coverage-policy.md) identifies
`Widget.sizeConstraints` as the last currently identified persistent compound-property gap after
[ADR 0183](0183-m8-combo-box-collection-and-optional-property-coverage.md) covered ComboBox collections
and optional selection. `SizeConstraints` already groups minimum, preferred and maximum size hints
in a small backend-neutral public value with equality, no pointers, runtime handles or ownership.

### Decision

Append the concrete `size_constraints` kind, preserving the six existing enum values. Declare exactly
one `sasd.ui.Widget.sizeConstraints` descriptor after visible, enabled and focusable. All Widget-derived
built-ins inherit that descriptor through metadata; the catalog grows from 21 to 22 properties while
the type catalog stays at 29. There is no generic compound/value carrier, `PropertyValue`, variant,
any, struct/record descriptor, field metadata or nested property path.

Use the existing public `sasd::ui::SizeConstraints` directly in typed runtime access, defaults and
snapshots. A duplicate tooling value would add mapping without a different semantic contract.
Minimum/preferred/maximum are not separate properties: the existing setter validates the minimum /
maximum relation atomically, avoiding inconsistent intermediate states and artificial restore ordering.

`readBuiltInSizeConstraintsProperty` validates copied descriptor identity and copies `sizeConstraints()`.
`writeBuiltInSizeConstraintsProperty` uses only `setSizeConstraints(value)`. True reports recognized,
compatible access, including same-value writes, rather than mutation. Invalid descriptors, kinds or
incompatible Components fail closed with nullopt/false. The setter's `std::invalid_argument` propagates
for minimum > maximum in either dimension, with the stored value unchanged. No validation is duplicated.
Preferred may remain outside min/max: measurement clamps it later. No additional non-negative coordinate
policy is introduced. Real changes invalidate measurement and propagate to parents through the setter;
same-value writes do not invalidate a valid cache. Property access never invalidates directly.

`builtInSizeConstraintsPropertyDefault(target_type_id, property)` returns `SizeConstraints{}` for Widget
and all compatible built-in descendants, resolved through the existing metadata hierarchy without
constructing an instance. No built-in constructor overrides this default. Minimum and preferred are
{0, 0}; maximum is {numeric_limits<Coordinate>::max(), numeric_limits<Coordinate>::max()}.
Unknown/non-Widget targets or malformed descriptors return nullopt.

`ComponentSizeConstraintsPropertySnapshot` owns declaring_type_id, property_id, a SizeConstraints value
and is_default. `BuiltInComponentPropertySnapshot.size_constraints_properties` is the seventh concrete
collection. Direct equality with the effective default determines is_default. Capture remains atomic
and owns all identity/value data across source mutation/destruction. A fresh Widget has its three
existing default booleans plus default sizeConstraints; Button/ComboBox retain their own properties
and the inherited Widget identity. Capture reads authoring configuration without running layout.

### Coverage review and boundaries

The review against all 29 built-in public APIs and ADR 0177 found no further persistent collection or
compound-property gaps in the current baseline. ComboBox items are owned collection configuration;
sizeConstraints is the remaining compound configuration. Form rows and Stack layers derive from
ordered ownership. Command/group/model/selection-model associations use reference edges. Model payloads
remain application data; viewports, selected rows/cells/nodes and tree expansion remain runtime state.
TextStyle remains presentation/style/resource state under a separate future policy.

Collection/compound property coverage can therefore be marked complete in both roadmaps. This does not
complete M8, serialization or reconstruction. Bounds, desiredSize, measure validity/caches and
MeasurementContext identity/revision remain derived; MeasureConstraints is parent runtime input,
not a Widget property. There is no serializer, persisted encoding, construction, reconstruction,
reference resolution, restore ordering engine, transaction, callback suppression or designer metadata.
The visible macOS/Cocoa M3 gate remains open. No release or tag is part of this slice.

### Local validation

Strict GCC 12.2 / WSL Debug and Release builds with warnings as errors passed. Full CTest passed
4/4 suites in both configurations: Core 481/481, Terminal 321/321, Rendered 129/129 and native POSIX
PTY smoke. The separate focused Debug run and independently compiled installed Release public-API
consumer each passed 66/66 property/geometry/layout tests. Both headless demos print the non-default
10x5 / 40x20 / 100x50 hints. Descriptor/enum compatibility, the unchanged 29-type catalog, relative
Markdown file links and diff whitespace were checked. Native Windows/macOS, sanitizers, SDL3, GTK
and AppKit are delegated to the existing CI matrices, not claimed as local executions.

## Deutsch

### Kontext

[ADR 0177](0177-m8-serializable-ui-description-state-coverage-policy.md) nennt `Widget.sizeConstraints`
als letzte aktuell identifizierte persistente Compound-Property-Lücke, nachdem ADR 0183 ComboBox-
Collections und optionale Selection abdeckt. `SizeConstraints` ist bereits ein kleiner backendneutraler
Public Value mit Equality, ohne Pointer, Runtime-Handles oder Ownership-Probleme.

### Entscheidung

Der konkrete Kind `size_constraints` wird nach den sechs bestehenden Enum-Werten angehängt.
`sasd.ui.Widget` deklariert genau einen Deskriptor `sizeConstraints` nach visible/enabled/focusable.
Alle abgeleiteten Built-in-Widgets erben ihn über die Metadatenhierarchie ohne Duplikate. Der Katalog
wächst von 21 auf 22 Properties bei weiterhin 29 Typen. Keine generischen Compound-/Value-Carrier,
PropertyValue, variant, any, Struct-/Record-Deskriptoren, Field-Metadaten oder Nested Paths.

Runtime-Zugriff, Defaults und Snapshots verwenden direkt `sasd::ui::SizeConstraints`; ein zweiter
identischer Tooling-Typ würde nur Mapping erzeugen. Minimum/preferred/maximum bleiben eine atomare
Property: der semantische Setter validiert minimum <= maximum gemeinsam und verhindert inkonsistente
Zwischenzustände sowie künstliche Restore-Reihenfolgen.

`readBuiltInSizeConstraintsProperty` prüft kopierte Deskriptoridentität und kopiert `sizeConstraints()`.
`writeBuiltInSizeConstraintsProperty` verwendet ausschließlich `setSizeConstraints(value)`. True bedeutet
kompatiblen Zugriff, auch bei gleichem Wert, nicht Wertänderung. Ungültige Identität/Kind/Component
liefert nullopt/false. Die `std::invalid_argument` des Setters bei minimum > maximum je Dimension
propagiert; der vorherige Wert bleibt erhalten. Keine doppelte Validierung. Preferred darf außerhalb
min/max liegen und wird erst beim Messen geclamped. Negative Koordinaten werden nicht zusätzlich
verboten. Änderungen invalidieren Measurement samt Parent-Propagation ausschließlich über den Setter;
Same-value-Writes erhalten gültige Caches. Property-Zugriff invalidiert nicht selbst.

`builtInSizeConstraintsPropertyDefault` liefert ohne Instanzkonstruktion über den geerbten Lookup für
Widget und alle kompatiblen Built-in-Ableitungen `SizeConstraints{}`. Kein Built-in-Konstruktor setzt
andere Hints. Minimum/preferred sind {0, 0}, maximum enthält zweimal numeric_limits<Coordinate>::max().
Unbekannte/non-Widget-Ziele und ungültige Deskriptoren liefern nullopt.

`ComponentSizeConstraintsPropertySnapshot` besitzt declaring_type_id, property_id, SizeConstraints und
is_default. `BuiltInComponentPropertySnapshot.size_constraints_properties` ist die siebte konkrete
Collection; direkte Value Equality zum effektiven Default bestimmt is_default. Capture bleibt atomar
und unabhängig von Mutation/Zerstörung der Quelle. Ein frisches Widget enthält drei bestehende
Default-Booleans plus Default-sizeConstraints; Button/ComboBox behalten eigene Properties und die
geerbte Widget-Identität. Capture liest Konfiguration ohne Layoutausführung.

### Coverage-Prüfung und Grenzen

Der Abgleich aller 29 öffentlichen Built-in-APIs mit ADR 0177 ergab keine weitere persistente Collection-
oder Compound-Property-Lücke der aktuellen Baseline. ComboBox.items ist besitzende Collection-
Konfiguration, sizeConstraints die verbleibende Compound-Konfiguration. Form-Rows und Stack-Layer
folgen geordneter Ownership; Command-/Group-/Model-/SelectionModel-Assoziationen sind Reference-Edges.
Model-Payloads bleiben Application Data, Viewports/aktuelle Selection/Tree-Expansion Runtime-Zustand.
TextStyle bleibt Presentation-/Style-/Resource-Zustand für eine spätere eigene Policy.

Der Collection-/Compound-Parent beider Roadmaps ist damit abgeschlossen, M8 und Serialisierung/
Rekonstruktion jedoch nicht. Bounds, desiredSize, Measure-Validität/-Caches und MeasurementContext-
Identität/Revision bleiben abgeleitet; MeasureConstraints bleibt Parent-Runtime-Input, keine Widget-
Property. Kein Serializer/Encoding, keine Construction/Reconstruction, Reference Resolution, Restore-
Engine, Transaktion, Callback-Unterdrückung oder Designer-Metadaten. Das sichtbare macOS-/Cocoa-M3-Gate
bleibt offen. Kein Release und kein Tag.

### Lokale Validierung

Strikte GCC-12.2-/WSL-Builds in Debug und Release mit Warnings-as-Errors bestanden. Vollständiges
CTest bestand jeweils 4/4 Suites: Core 481/481, Terminal 321/321, Rendered 129/129 und nativer POSIX-
PTY-Smoke. Separater fokussierter Debug-Lauf und unabhängig gebauter Consumer der installierten
Release-Public-API bestanden jeweils 66/66 Property-/Geometry-/Layout-Tests. Beide headless Demos
zeigen die non-default Hints 10x5 / 40x20 / 100x50. Descriptor-/Enum-Kompatibilität, unveränderter
29-Typen-Katalog, relative Markdown-Dateilinks und Diff-Whitespace wurden geprüft. Native Plattformen,
Sanitizer, SDL3, GTK und AppKit prüfen die bestehenden CI-Matrizen; dies sind keine lokalen Läufe.
