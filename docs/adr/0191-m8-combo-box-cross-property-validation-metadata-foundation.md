# ADR 0191: M8 ComboBox Cross-Property Validation Metadata Foundation

- Status: Accepted
- Date: 2026-10-29

## English

### Context

ADRs 0189 and 0190 made static scalar domains, string normalization and atomic compound-value
validation discoverable from one exact property descriptor. The remaining built-in validator rule
for `ComboBox.selectedIndex` depends on a second property of the same node:
`selectedIndex == nullopt || selectedIndex < items.size()`. A property inspector should be able to
discover that relation without constructing a ComboBox, while the format-neutral document validator
should not identify ComboBox by a hardcoded type branch.

This is not the same kind of rule as RadioGroup exclusivity. ComboBox validation reads two
properties of one component. RadioGroup exclusivity reads several RadioButton nodes, reference
edges, a RadioGroup target and their selected values. The latter is a graph/reference invariant and
requires a separate future design.

### Decision

Extend `component_property_validation_metadata.hpp` with the concrete
`OptionalUnsignedIntegerCrossPropertyValidationKind::index_within_string_collection` relation,
the `OptionalUnsignedIntegerCrossPropertyValidation` value and
`builtInOptionalUnsignedIntegerCrossPropertyValidation()`. The sole current entry is:

- source: `sasd.ui.ComboBox.selectedIndex` / `optional_unsigned_integer`;
- relation: `index_within_string_collection`;
- dependency: the exact `sasd.ui.ComboBox.items` / `string_collection` catalog descriptor.

The dependency is a complete `ComponentPropertyMetadata` value, not a loose `"items"` string, a
runtime pointer or a Component instance. Both input and dependency therefore use stable,
case-sensitive declaring-type/property/kind identity. A copied valid source descriptor succeeds;
unknown, wrong-type, wrong-property, wrong-kind, wrong-case and fabricated-enum descriptors return
`std::nullopt`. The implementation also verifies that the resolved dependency is an exact
`string_collection` descriptor of the same declaring type.

An absent optional value means no selection and is valid for every collection, including an empty
one. A present `uint64_t` value is valid exactly when it denotes an existing collection element.
Document validation uses a C++20-safe mixed-width comparison and never narrows the persistent value
to host `size_t`. Host representability remains the responsibility of typed runtime access and
reconstruction.

The common document validator resolves each optional-unsigned descriptor, requires engaged
cross-property metadata, finds the stored dependency in the same node by complete descriptor
identity, and applies the named relation. Missing metadata or a missing/wrong-kind dependency fails
closed. Input array order and vector position have no semantic role. Invalid values are rejected;
they are not clamped, cleared or normalized. The hardcoded `node.type_id == "sasd.ui.ComboBox"`
branch is removed.

`ComboBox::setSelectedIndex()` remains the authoritative runtime enforcement boundary and keeps its
out-of-range exception. `ComboBox::setItems()` behavior is unchanged. Property declaration and
reconstruction order continue to restore `items` before `selectedIndex`; validation metadata does
not become a restore scheduler or topological dependency graph. JSON v1 syntax, members, canonical
bytes and format version remain unchanged.

### Consequences and boundaries

Tooling can now recognize that `selectedIndex` is not an arbitrary optional number. A future
property inspector may offer `none`, limit an index editor to existing items or mark an invalid
value before calling a setter. This slice implements none of those UI features.

No generic `ValidationRule`, expression AST, property-path DSL, predicate callback, `std::function`,
variant rule value, operator string or dependency graph is introduced. The concrete relation can be
extended only when another real pattern provides evidence. RadioGroup exclusivity and the existing
reference source/target compatibility table remain concrete graph validation for a later
reference/graph-metadata slice. Designer-friendly attributes, resources and IDE integration also
remain open.

## Deutsch

### Kontext

ADRs 0189 und 0190 machten statische Skalarbereiche, String-Normalisierung und atomare Compound-
Value-Validierung über einen exakten Property-Deskriptor auffindbar. Die verbleibende Built-in-
Validatorregel für `ComboBox.selectedIndex` hängt von einer zweiten Property desselben Nodes ab:
`selectedIndex == nullopt || selectedIndex < items.size()`. Ein Property Inspector soll diesen
Zusammenhang ohne ComboBox-Instanz erkennen können; zugleich soll der formatneutrale Document
Validator ComboBox nicht über einen hardcodierten Typzweig identifizieren.

Diese Regel unterscheidet sich grundlegend von RadioGroup-Exklusivität. ComboBox-Validierung liest
zwei Properties einer Component. RadioGroup-Exklusivität benötigt mehrere RadioButton-Nodes,
Reference Edges, ein RadioGroup-Ziel und deren Selected-Werte. Letzteres ist eine Graph-/Reference-
Invariante und braucht einen getrennten späteren Entwurf.

### Entscheidung

`component_property_validation_metadata.hpp` erhält die konkrete Relation
`OptionalUnsignedIntegerCrossPropertyValidationKind::index_within_string_collection`, den Value-Typ
`OptionalUnsignedIntegerCrossPropertyValidation` und die Abfrage
`builtInOptionalUnsignedIntegerCrossPropertyValidation()`. Der einzige heutige Eintrag ist:

- Quelle: `sasd.ui.ComboBox.selectedIndex` / `optional_unsigned_integer`;
- Relation: `index_within_string_collection`;
- Dependency: der exakte Katalogdeskriptor `sasd.ui.ComboBox.items` / `string_collection`.

Die Dependency ist ein vollständiger `ComponentPropertyMetadata`-Wert und kein loser `"items"`-
String, Runtime-Pointer oder Component-Objekt. Input und Dependency verwenden damit stabile,
case-sensitive Declaring-Type-/Property-/Kind-Identität. Eine gültige Deskriptorkopie funktioniert;
unbekannte, falsche Typ-, Property-, Kind- oder Case-Werte sowie fabrizierte Enums liefern
`std::nullopt`. Die Implementierung prüft zusätzlich, dass die aufgelöste Dependency ein exakter
`string_collection`-Deskriptor desselben deklarierenden Typs ist.

Ein fehlender optionaler Wert bedeutet keine Auswahl und ist für jede Collection einschließlich
einer leeren gültig. Ein vorhandener `uint64_t`-Wert ist genau dann gültig, wenn er ein vorhandenes
Collection-Element bezeichnet. Die Dokumentvalidierung verwendet einen C++20-sicheren Vergleich
unterschiedlicher Breiten und verengt den persistenten Wert nie auf Host-`size_t`. Host-
Darstellbarkeit bleibt Aufgabe des typisierten Runtime-Zugriffs und der Rekonstruktion.

Der gemeinsame Document Validator löst jeden Optional-Unsigned-Deskriptor auf, verlangt vorhandene
Cross-Property-Metadata, findet die gespeicherte Dependency im selben Node über vollständige
Deskriptoridentität und wendet die benannte Relation an. Fehlende Metadata oder eine fehlende bzw.
falsch typisierte Dependency arbeitet fail-closed. Input-Array-Reihenfolge und Vectorposition haben
keine Semantik. Ungültige Werte werden abgelehnt, nicht geklemmt, gelöscht oder normalisiert. Der
hardcodierte Zweig `node.type_id == "sasd.ui.ComboBox"` entfällt.

`ComboBox::setSelectedIndex()` bleibt die autoritative Runtime-Enforcement-Grenze mit seiner
Out-of-range-Exception. Das Verhalten von `ComboBox::setItems()` bleibt unverändert. Property-
Deklaration und Rekonstruktionsreihenfolge restaurieren weiterhin `items` vor `selectedIndex`;
Validation Metadata wird weder Restore-Scheduler noch topologischer Dependency Graph. JSON-v1-
Syntax, Member, kanonische Bytes und Formatversion bleiben unverändert.

### Folgen und Grenzen

Tooling kann nun erkennen, dass `selectedIndex` keine beliebige optionale Zahl ist. Ein zukünftiger
Property Inspector könnte `none` anbieten, einen Index-Editor auf vorhandene Items begrenzen oder
einen ungültigen Zustand vor dem Setter-Aufruf markieren. Dieser Slice implementiert keine dieser
UI-Funktionen.

Es entsteht keine generische `ValidationRule`, Expression-AST, Property-Path-DSL, Predicate-
Callback-, `std::function`-, Variant-Rule-Value-, Operator-String- oder Dependency-Graph-
Abstraktion. Der konkrete Relationstyp wird erst bei einem weiteren realen Muster evidenzbasiert
ergänzt. RadioGroup-Exklusivität und die bestehende Tabelle zur Reference-Source-/Target-
Kompatibilität bleiben konkrete Graphvalidierung für einen späteren Reference-/Graph-Metadata-
Slice. Designer-freundliche Attribute, Resources und IDE-Integration bleiben ebenfalls offen.
