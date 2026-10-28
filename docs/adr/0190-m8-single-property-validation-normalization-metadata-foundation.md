# ADR 0190: M8 Single-Property Validation and Normalization Metadata Foundation

- Status: Accepted
- Date: 2026-10-28

## English

### Context

ADR 0189 made the current static integer domains discoverable without constructing Components. Two
other rules in the built-in baseline are also determined by one exact property descriptor and one
value: `TextField.text` has single-line UTF-8 normalization, and `Widget.sizeConstraints` requires
each minimum dimension not to exceed its maximum. Tooling and the format-neutral document validator
should share these contracts without turning the metadata layer into a general rule engine.

These contracts are different in kind. `Widget::setSizeConstraints()` rejects an invalid range with
`std::invalid_argument`. `TextField::setText()` accepts arbitrary bytes and applies
`utf8::sanitizeSingleLine()`: malformed UTF-8 becomes U+FFFD, C0/C1 controls and Unicode line or
paragraph separators are removed, and the canonical result is stored. Describing the latter as an
invalid-value range would misrepresent the runtime API.

### Decision

Extend `component_property_validation_metadata.hpp` with two small typed families:

- `StringPropertyNormalizationKind::{identity, single_line_utf8}` and
  `builtInStringPropertyNormalization()`;
- `SizeConstraintsPropertyValidationKind::minimum_not_greater_than_maximum` and
  `builtInSizeConstraintsPropertyValidation()`.

Both queries validate complete case-sensitive descriptor value identity—declaring type, property ID
and value kind. A copied valid descriptor succeeds; unknown, fabricated, wrong-kind and wrong-case
descriptors fail closed with `std::nullopt`. Catalog pointer identity is irrelevant and no Component
is constructed.

Every current string descriptor makes an explicit normalization decision:

- `sasd.ui.Command.text`: `identity`;
- `sasd.ui.Label.text`: `identity`;
- `sasd.ui.Button.text`: `identity`;
- `sasd.ui.TextField.text`: `single_line_utf8`;
- `sasd.ui.CheckBox.text`: `identity`;
- `sasd.ui.RadioButton.text`: `identity`.

`identity` is engaged metadata, not absence of metadata. This distinguishes a known string property
which performs no additional toolkit normalization from an invalid descriptor and forces future
string properties to choose deliberately. It does not allow malformed UTF-8: document/JSON UTF-8
validity remains a separate format invariant.

The sole current `size_constraints` descriptor, `sasd.ui.Widget.sizeConstraints`, uses
`minimum_not_greater_than_maximum`, exactly matching `SizeConstraints::hasValidRange()`. It constrains
only minimum versus maximum on each axis. Preferred may remain outside that interval and negative
values remain legal. No stronger design-time rule is invented.

Runtime setters remain authoritative and unchanged; they do not consult metadata. The common
format-neutral document validator resolves each exact descriptor, requires its policy, and fails
closed when policy metadata is missing. Valid UTF-8 string values then apply `identity` or require
`utf8::sanitizeSingleLine(value) == value`. Size-constraints values execute `hasValidRange()` only
under their descriptor policy. The hardcoded `TextField` type branch and the implicit rule for every
`size_constraints` kind are removed. Persisted state is rejected, never silently normalized.

Catalog tests require policies for all directly declared built-in string and size-constraints
descriptors. JSON v1 syntax, fields, version, canonical bytes and reconstruction order do not change.

### Consequences and boundaries

Property schema, effective defaults, static numeric validation metadata, single-property
normalization/validation metadata, typed runtime access, document validation and runtime setter
enforcement remain distinct layers. Instance-free tooling can display or apply a named policy while
runtime objects remain independently correct.

No `ValidationRule`, rule AST, variant rule value, predicate callback, expression language or DSL is
introduced. `ComboBox.selectedIndex < items.size()` remains a concrete cross-property validator rule;
RadioGroup exclusivity remains a graph/reference invariant. TextStyle color fields are not property
descriptors. Designer-friendly attributes, resource metadata and IDE integration remain future work.

## Deutsch

### Kontext

ADR 0189 machte die heutigen statischen Integer-Domänen ohne Component-Konstruktion auffindbar. Zwei
weitere Regeln der Built-in-Baseline hängen ebenfalls nur von einem exakten Property-Deskriptor und
einem Wert ab: `TextField.text` besitzt Single-Line-UTF-8-Normalisierung und
`Widget.sizeConstraints` verlangt, dass jede Minimum-Dimension ihr Maximum nicht überschreitet.
Tooling und formatneutraler Document Validator sollen diese Verträge gemeinsam verwenden, ohne die
Metadatenschicht zu einer allgemeinen Rule Engine auszubauen.

Die Verträge sind begrifflich verschieden. `Widget::setSizeConstraints()` lehnt eine ungültige Range
mit `std::invalid_argument` ab. `TextField::setText()` akzeptiert beliebige Bytes und wendet
`utf8::sanitizeSingleLine()` an: fehlerhaftes UTF-8 wird U+FFFD, C0-/C1-Controls sowie Unicode-Line-
und Paragraph-Separatoren werden entfernt und das kanonische Ergebnis wird gespeichert. Eine
Invalid-Value-Range würde diese Runtime-Semantik falsch darstellen.

### Entscheidung

`component_property_validation_metadata.hpp` erhält zwei kleine typisierte Familien:

- `StringPropertyNormalizationKind::{identity, single_line_utf8}` und
  `builtInStringPropertyNormalization()`;
- `SizeConstraintsPropertyValidationKind::minimum_not_greater_than_maximum` und
  `builtInSizeConstraintsPropertyValidation()`.

Beide Abfragen validieren die vollständige case-sensitive Deskriptorwert-Identität aus Declaring
Type, Property-ID und Value Kind. Eine gültige Kopie funktioniert; unbekannte, erfundene, falsch
typisierte oder falsch geschriebene Deskriptoren liefern fail-closed `std::nullopt`.
Katalog-Pointeridentität ist irrelevant und keine Component wird konstruiert.

Jeder heutige String-Deskriptor trifft eine explizite Normalisierungsentscheidung:

- `sasd.ui.Command.text`: `identity`;
- `sasd.ui.Label.text`: `identity`;
- `sasd.ui.Button.text`: `identity`;
- `sasd.ui.TextField.text`: `single_line_utf8`;
- `sasd.ui.CheckBox.text`: `identity`;
- `sasd.ui.RadioButton.text`: `identity`.

`identity` ist vorhandene Metadata, nicht fehlende Metadata. So bleibt eine bekannte String-Property
ohne zusätzliche Toolkit-Normalisierung von einem ungültigen Deskriptor unterscheidbar und künftige
String-Properties müssen bewusst entscheiden. Malformed UTF-8 wird dadurch nicht erlaubt: UTF-8-
Gültigkeit von Document/JSON bleibt eine getrennte Format-Invariante.

Der einzige heutige `size_constraints`-Deskriptor `sasd.ui.Widget.sizeConstraints` verwendet
`minimum_not_greater_than_maximum`, exakt entsprechend `SizeConstraints::hasValidRange()`. Nur
Minimum gegen Maximum je Achse ist eingeschränkt. Preferred darf außerhalb liegen und negative Werte
bleiben legal. Es entsteht keine stärkere Design-Time-Regel.

Runtime-Setter bleiben autoritativ und unverändert; sie fragen keine Metadata ab. Der gemeinsame
formatneutrale Document Validator löst jeden exakten Deskriptor auf, verlangt seine Policy und
arbeitet bei fehlender Metadata fail-closed. Nach allgemeiner UTF-8-Prüfung gilt `identity` oder
`utf8::sanitizeSingleLine(value) == value`. Size-Constraints-Werte führen `hasValidRange()` nur unter
ihrer Deskriptorpolicy aus. Der hardcodierte `TextField`-Typzweig und die implizite Regel für jeden
`size_constraints`-Kind entfallen. Persistierter Zustand wird abgelehnt und nie still normalisiert.

Katalogtests verlangen Policies für alle direkt deklarierten Built-in-String- und Size-Constraints-
Deskriptoren. JSON-v1-Syntax, Felder, Version, kanonische Bytes und Rekonstruktionsreihenfolge bleiben
unverändert.

### Folgen und Grenzen

Property Schema, effektive Defaults, statische numerische Validation Metadata, Single-Property-
Normalization/-Validation-Metadata, typisierter Runtime-Zugriff, Document Validation und Runtime-
Setter-Enforcement bleiben getrennte Schichten. Instanzfreies Tooling kann eine benannte Policy
anzeigen oder anwenden, während Runtime-Objekte unabhängig korrekt bleiben.

Es entstehen weder `ValidationRule`, Rule AST, Variant-Rule-Value, Predicate Callback,
Expression-Sprache noch DSL. `ComboBox.selectedIndex < items.size()` bleibt eine konkrete Cross-
Property-Regel des Validators; RadioGroup-Exklusivität bleibt Graph-/Reference-Invariante.
TextStyle-Color-Felder sind keine Property-Deskriptoren. Designer-freundliche Attribute,
Resource-Metadaten und IDE-Integration bleiben spätere Arbeit.
