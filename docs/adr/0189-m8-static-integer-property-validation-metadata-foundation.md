# ADR 0189: M8 Static Integer Property Validation Metadata Foundation

- Status: Accepted
- Date: 2026-10-28

## English

### Context

M8 Slices 1 through 18 complete the current built-in serialization loop from runtime graph through
capture, canonical JSON v1, parsing, validated format-neutral document and reconstruction into an
independent runtime graph. The property catalog already distinguishes `signed_integer` and
`unsigned_integer`, while typed runtime access and the concrete layout setters enforce the seven
current integer-property domains.

Tooling cannot discover those domains from schema metadata today. A property inspector would need to
construct Components, probe setters and interpret exceptions, or duplicate toolkit rules. The first
design-time validation layer should therefore follow serialization, but remain smaller than a generic
validation-rule system: only domains that are static from a complete property descriptor and require
neither an instance nor another property value belong in this slice.

### Decision

Add the public, backend-neutral `sasd/ui/component_property_validation_metadata.hpp` layer. It
contains separate `SignedIntegerPropertyRange` and `UnsignedIntegerPropertyRange` inclusive value
types and typed built-in queries. The split preserves the existing `std::int64_t` / `std::uint64_t`
property-carrier boundary; no `variant`, `any`, generic numeric value or floating conversion is added.

Queries validate the complete case-sensitive descriptor value: `declaring_type_id`, `property_id`
and `value_kind`. Catalog address is not identity, so a copied valid descriptor succeeds. Unknown,
fabricated, wrong-kind, wrong-case and mismatched sibling identities fail closed with `std::nullopt`.
The API does not construct or retain a Component.

The six current signed ranges are inclusive `[0, Coordinate::max]`:

- `sasd.ui.HBox.spacing`;
- `sasd.ui.VBox.spacing`;
- `sasd.ui.GridLayout.columnSpacing`;
- `sasd.ui.GridLayout.rowSpacing`;
- `sasd.ui.FormLayout.columnSpacing`;
- `sasd.ui.FormLayout.rowSpacing`.

The persistent carrier can hold the wider `int64_t` domain, but these semantic properties use the
fixed-width public `Coordinate` type. Advertising `Coordinate::max` prevents tooling from offering a
value that no layout instance can represent. The zero minimum describes the existing non-negative
setter contract.

`sasd.ui.GridLayout.columnCount` has the inclusive range `[1, UINT64_MAX]`. Its portable semantic
rule is `columnCount > 0`, and its persisted carrier is `uint64_t`. The maximum is deliberately not
`size_t::max`: host representability remains a typed property-access and reconstruction concern. A
value above 32-bit `size_t` may therefore be semantically valid yet not reconstructible on that host,
preserving the boundary established by ADRs 0187 and 0188.

Concrete runtime setters remain authoritative and unchanged. They must enforce valid runtime API use
without metadata lookup. Validation metadata describes the same static contract for instance-free
consumers; it neither replaces setter checks nor introduces clamping, correction or new exceptions.

The common internal format-neutral document validator reuses these queries for every signed- and
unsigned-integer document property. The former hardcoded `GridLayout.columnCount == 0` check is
removed. Negative spacing and signed values above `Coordinate::max` now fail with the existing
`invalid_property_value` JSON parse error before reconstruction. `UINT64_MAX` column count remains
semantically valid during document validation; a narrower host may reject it later as
`value_not_representable` during reconstruction.

Catalog tests enumerate every built-in type and directly declared property. Every current
`signed_integer` descriptor must have exactly the signed query engaged, every current
`unsigned_integer` descriptor exactly the unsigned query, and all other kinds neither. This makes a
future integer property require an explicit validation-metadata decision.

### Consequences and boundaries

Schema, effective defaults, static validation metadata and typed runtime access are separate layers.
A headless metadata consumer can display an integer domain without constructing a Component, while
runtime APIs remain independently correct. JSON v1 fields, version and canonical bytes do not change.

This is not a rule engine. `ComboBox.selectedIndex < items.size()` remains a contextual cross-property
rule; `Widget.sizeConstraints` remains one compound value with its own invariant; TextField
single-line sanitization remains a concrete text policy; and RadioGroup exclusivity remains a graph
invariant. The shared validator keeps those established concrete checks. No dependency expression,
message/localization system, designer UI, property editor, resource validation, category, display
name or IDE integration is introduced.

## Deutsch

### Kontext

M8 Slices 1 bis 18 schließen den aktuellen Built-in-Serialisierungskreis vom Runtime-Graph über
Capture, kanonisches JSON v1, Parsing und validiertes formatneutrales Dokument bis zur Rekonstruktion
eines unabhängigen Runtime-Graphs. Der Property-Katalog unterscheidet bereits `signed_integer` und
`unsigned_integer`; typisierter Runtime-Zugriff und konkrete Layout-Setter erzwingen die sieben
heutigen Integer-Property-Domänen.

Tooling kann diese Domänen bisher nicht aus Schema-Metadaten ermitteln. Ein Property Inspector müsste
Components konstruieren, Setter probeweise aufrufen und Exceptions interpretieren oder Toolkit-Regeln
duplizieren. Die erste Design-Time-Validierungsschicht folgt deshalb auf die abgeschlossene
Serialisierung, bleibt aber bewusst kleiner als ein generisches Regelsystem: In diesen Slice gehören
nur Domänen, die allein aus einem vollständigen Property-Deskriptor ohne Instanz und ohne andere
Property-Werte bestimmt werden können.

### Entscheidung

Die öffentliche backendneutrale Schicht `sasd/ui/component_property_validation_metadata.hpp` wird
ergänzt. Sie enthält getrennte inklusive Value Types `SignedIntegerPropertyRange` und
`UnsignedIntegerPropertyRange` sowie typisierte Built-in-Abfragen. Die Trennung erhält die vorhandene
`std::int64_t`-/`std::uint64_t`-Grenze; es entstehen weder `variant`, `any`, generischer Numeric Value
noch Floating-Konvertierung.

Abfragen validieren den vollständigen, case-sensitiven Deskriptorwert aus `declaring_type_id`,
`property_id` und `value_kind`. Katalogadressen sind keine Identität; eine gültige Kopie funktioniert.
Unbekannte, erfundene, falsch typisierte, falsch geschriebene oder zwischen Geschwistern vermischte
Identitäten liefern fail-closed `std::nullopt`. Die API konstruiert und hält keine Component.

Die sechs aktuellen Signed-Bereiche sind inklusive `[0, Coordinate::max]`:

- `sasd.ui.HBox.spacing`;
- `sasd.ui.VBox.spacing`;
- `sasd.ui.GridLayout.columnSpacing`;
- `sasd.ui.GridLayout.rowSpacing`;
- `sasd.ui.FormLayout.columnSpacing`;
- `sasd.ui.FormLayout.rowSpacing`.

Der persistente Carrier kann die größere `int64_t`-Domäne tragen, diese semantischen Properties
verwenden aber den öffentlichen Fixed-Width-Typ `Coordinate`. `Coordinate::max` verhindert, dass
Tooling Werte anbietet, die keine Layout-Instanz darstellen kann. Das Minimum null beschreibt den
bestehenden nicht-negativen Setter-Vertrag.

`sasd.ui.GridLayout.columnCount` besitzt den inklusiven Bereich `[1, UINT64_MAX]`. Die portable
semantische Regel lautet `columnCount > 0`, der persistente Carrier ist `uint64_t`. Das Maximum ist
bewusst nicht `size_t::max`: Host-Darstellbarkeit bleibt Aufgabe des typisierten Property-Zugriffs
und der Rekonstruktion. Ein Wert oberhalb eines 32-Bit-`size_t` kann daher semantisch gültig, auf
diesem Host aber nicht rekonstruierbar sein; die Grenze aus ADR 0187/0188 bleibt erhalten.

Konkrete Runtime-Setter bleiben autoritativ und unverändert. Sie müssen Runtime-API-Aufrufe ohne
Metadaten-Lookup korrekt validieren. Die Validation Metadata beschreibt denselben statischen Vertrag
für instanzfreie Consumer; sie ersetzt keine Setter-Prüfung und führt weder Clamping, Korrektur noch
neue Exceptions ein.

Der gemeinsame interne formatneutrale Document Validator verwendet die Abfragen für jede Signed- und
Unsigned-Integer-Document-Property. Der bisher hardcodierte Sonderfall
`GridLayout.columnCount == 0` entfällt. Negatives Spacing und Signed-Werte oberhalb
`Coordinate::max` liefern nun bereits beim Parsing den bestehenden Fehler `invalid_property_value`.
`UINT64_MAX` als columnCount bleibt semantisch gültig; ein schmalerer Host kann den Wert erst bei der
Rekonstruktion als `value_not_representable` ablehnen.

Katalogtests iterieren jeden Built-in-Typ und jede direkt deklarierte Property. Jeder heutige
`signed_integer`-Deskriptor muss ausschließlich eine Signed Range, jeder `unsigned_integer`-
Deskriptor ausschließlich eine Unsigned Range und jeder andere Kind keine Integer Range besitzen.
Eine zukünftige Integer-Property erzwingt damit eine explizite Metadatenentscheidung.

### Folgen und Grenzen

Schema, effektive Defaults, statische Validation Metadata und typisierter Runtime-Zugriff bleiben
getrennte Schichten. Ein Headless-Metadatenconsumer kann die Integer-Domäne ohne Component-
Konstruktion darstellen; Runtime-APIs bleiben unabhängig korrekt. JSON-v1-Felder, Version und
kanonische Bytes ändern sich nicht.

Dies ist keine Rule Engine. `ComboBox.selectedIndex < items.size()` bleibt eine kontextabhängige
Cross-Property-Regel, `Widget.sizeConstraints` ein atomarer Compound-Wert mit eigener Invariante,
TextField-Single-Line-Sanitization eine konkrete Textpolicy und RadioGroup-Exklusivität eine Graph-
Invariante. Der gemeinsame Validator behält diese bestehenden konkreten Prüfungen. Dependency-
Expressions, Meldungs-/Lokalisierungssystem, Designer-UI, Property Editor, Ressourcenvalidierung,
Kategorien, Display Names und IDE-Integration werden nicht eingeführt.
