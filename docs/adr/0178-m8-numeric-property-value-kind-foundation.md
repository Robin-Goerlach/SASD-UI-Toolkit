# ADR 0178: M8 Numeric Property Value-Kind Foundation

- Status: Accepted
- Date: 2026-10-23

## Predecessors

- [0172: Property schema](0172-m8-component-property-metadata-schema-foundation.md)
- [0173: Typed runtime access](0173-m8-typed-runtime-property-value-access-foundation.md)
- [0174: Effective defaults](0174-m8-effective-property-default-metadata-foundation.md)
- [0176: Owned snapshots](0176-m8-built-in-component-property-snapshot-foundation.md)
- [0177: State coverage policy](0177-m8-serializable-ui-description-state-coverage-policy.md)

## English

### Context

The verified Slice-7 baseline has 26 built-in types and ten boolean/string property descriptors.
The state coverage policy identifies seven existing numeric layout properties as durable authoring
configuration. Their C++ APIs use `Coordinate` (`std::int32_t`) and `std::size_t`; these implementation
types must not define the public tooling value representation.

### Decision

Add `ComponentPropertyValueKind::signed_integer` and `unsigned_integer`, with separate typed
`std::int64_t` and `std::uint64_t` APIs. The catalog grows to seventeen descriptors:

| Declaring type | Property | Kind | Default |
|---|---|---|---:|
| `sasd.ui.HBox` | `spacing` | signed_integer | 0 |
| `sasd.ui.VBox` | `spacing` | signed_integer | 0 |
| `sasd.ui.GridLayout` | `columnCount` | unsigned_integer | 1 |
| `sasd.ui.GridLayout` | `columnSpacing` | signed_integer | 0 |
| `sasd.ui.GridLayout` | `rowSpacing` | signed_integer | 0 |
| `sasd.ui.FormLayout` | `columnSpacing` | signed_integer | 1 |
| `sasd.ui.FormLayout` | `rowSpacing` | signed_integer | 0 |

`readBuiltInSignedIntegerProperty` / `readBuiltInUnsignedIntegerProperty` return typed optionals;
`writeBuiltInSignedIntegerProperty` / `writeBuiltInUnsignedIntegerProperty` return access success,
including same-value writes. Validate the complete descriptor identity by value against the immutable
catalog and check Component compatibility internally. A copied valid descriptor is valid; sibling
properties with the same local ID remain distinct. No HBox descriptor may address VBox spacing, nor
may Grid spacing descriptors address Form spacing.

Both conversion directions use C++20 `std::in_range` before casting. An unrepresentable read returns
`std::nullopt`; an unrepresentable write returns false before mutation. Domain validation belongs
exclusively to the semantic setter: representable negative spacing and zero column count propagate
the existing `std::invalid_argument`. No duplicate validation or exception translation is added.

`builtInSignedIntegerPropertyDefault` / `builtInUnsignedIntegerPropertyDefault` validate the exact
target type and descriptor identity, then return immutable constants matching public construction
defaults. They construct no Component.

Snapshots own separate `signed_integer_properties` and `unsigned_integer_properties`, using
`ComponentSignedIntegerPropertySnapshot` / `ComponentUnsignedIntegerPropertySnapshot` entries with
owned `declaring_type_id`, `property_id`, typed `value` and `is_default`. Base-to-derived traversal
and declaration order within each collection remain unchanged. Any failed read or default query
rejects the entire snapshot. Capturing catalogued properties still does not promise serialization
or complete UI-description coverage.

### Alternatives and consequences

Exposing `Coordinate` or `size_t` would couple tooling to layout implementation or host word size.
A generic `PropertyValue`, `std::variant` or `std::any` is unnecessary for these explicit consumers.
Separate typed APIs and collections extend the established boolean/string contract without adding
a universal value abstraction, dependency, backend policy or object graph.

RadioButton properties, ComboBox optional/collection state, compound values, style/resources,
references, ownership reconstruction, validation metadata, serializer, encoding, deserializer,
factory and designer remain outside this slice. M8 remains active; serialization remains open.
The recommended next slice is **M8 Slice 9 – RadioButton Scalar Property Coverage** (`text`,
`selected`), without implementing it here. The macOS/Cocoa M3 gate remains open. No release or tag.

## Deutsch

### Kontext

Der verifizierte Slice-7-Ausgangsstand enthält 26 Built-in-Typen und zehn Boolean-/String-
Deskriptoren. Die Coverage-Policy klassifiziert sieben bestehende numerische Layout-Properties als
dauerhafte Authoring-Konfiguration. Die C++-APIs verwenden `Coordinate` (`std::int32_t`) und
`std::size_t`; diese Implementierungstypen sollen nicht die öffentliche Tooling-Repräsentation sein.

### Entscheidung

`signed_integer` und `unsigned_integer` erweitern `ComponentPropertyValueKind`. Separate typisierte
APIs verwenden `std::int64_t` bzw. `std::uint64_t`. Die Tabelle oben definiert alle sieben neuen
Properties samt deklarierendem Typ, Kind und Default; der Katalog wächst auf siebzehn Deskriptoren.

Die Signed-/Unsigned-Read-Funktionen liefern typisierte Optionals, die Write-Funktionen melden
Zugriffserfolg einschließlich Same-Value-Writes. Vollständige Deskriptoridentität wird nach Wert
gegen den unveränderlichen Katalog geprüft, Component-Kompatibilität intern sicher festgestellt.
Kopierte gültige Deskriptoren bleiben gültig. Gleiche lokale Property-IDs verschiedener Typen sind
verschieden: HBox adressiert kein VBox-Spacing, Grid-Spacing adressiert kein Form-Spacing.

Beide Konvertierungsrichtungen prüfen vor dem Cast mit C++20 `std::in_range` die Darstellbarkeit.
Nicht darstellbare Reads liefern `std::nullopt`; Writes liefern false ohne Mutation. Domain-
Validierung bleibt ausschließlich beim semantischen Setter. Darstellbares negatives Spacing und
Spaltenzahl null propagieren dessen bestehende `std::invalid_argument`; keine doppelte Prüfung und
keine Exception-Übersetzung. Numerische Default-Queries prüfen exakten Zieltyp und vollständige
Property-Identität und liefern unveränderliche Konstanten ohne Component-Instanziierung.

Snapshots besitzen getrennte `signed_integer_properties` und `unsigned_integer_properties` mit
`ComponentSignedIntegerPropertySnapshot` bzw. `ComponentUnsignedIntegerPropertySnapshot`. Jeder
Eintrag besitzt `declaring_type_id`, `property_id`, typisierten `value` und `is_default`. Basis-zu-
Derived-Traversierung und Deklarationsreihenfolge innerhalb jeder Collection bleiben erhalten.
Fehlt ein Read oder Default, scheitert der gesamte Snapshot. Erfolg bedeutet weiterhin nur die
Erfassung katalogisierter Properties, keine vollständige UI-Description oder Serialisierung.

### Alternativen und Folgen

`Coordinate` oder `size_t` als Tooling-Typ würden Layout-Implementierung bzw. Host-Wortbreite
exponieren. Ein generischer `PropertyValue`, `std::variant` oder `std::any` ist für diese Consumer
unnötig. Getrennte APIs und Collections erweitern den bisherigen Vertrag ohne universelle Value-
Abstraktion, neue Abhängigkeit, Backend-Policy oder Objektgraph.

RadioButton-Properties, optionale/Collection-Werte der ComboBox, Compound-Werte, Style/Resources,
References, Ownership-Rekonstruktion, Validation-Metadaten, Serializer, Encoding, Deserializer,
Factory und Designer bleiben außerhalb. M8 bleibt aktiv und Serialisierung offen. Empfohlen wird
**M8 Slice 9 – RadioButton Scalar Property Coverage** (`text`, `selected`), hier nicht implementiert.
Das macOS/Cocoa-M3-Gate bleibt offen. Kein Release und kein Tag.

## Local validation / Lokale Validierung

Recovery verified local branches, remote heads, working/staged diffs, history, reflog and existing
PRs before implementation. No prior Slice-8 work existed. The exact base remained
`3295808a463970930c922b28f93d0e1a1fdda6b1`; its complete CI was successful. Unrelated local notes
were preserved. No existing history was reset or rewritten.

Validation used the installed Debian WSL GCC 12.2.0 / CMake toolchain, with no new dependency:

- strict Debug and Release builds with tests/examples enabled and warnings as errors;
- complete Debug and Release CTest suites: 4/4 each (Core, Terminal, Rendered, POSIX PTY);
- focused Core CTest and four standalone numeric regression cases;
- Debug and Release Component Metadata demos, including numeric schema/access/defaults/snapshots;
- `git diff --check`, staged diff review and 209 relative documentation links without broken links;
- ADR numbering/index validation.

Native Windows/macOS, sanitizers, SDL3, GTK and AppKit are validated by the configured CI rather
than claimed as local runs. PR-head and exact merge-SHA CI results are reported with the pull request.

Die Recovery prüfte lokale und Remote-Branches, Working-/Staging-Diffs, History, Reflog und PRs.
Vorherige Slice-8-Arbeit war nicht vorhanden; der exakte Ausgangsstand oben und dessen vollständige
grüne CI wurden bestätigt. Fremde lokale Notizen blieben erhalten, ohne Reset oder History-Rewrite.
Die vorhandene Debian-WSL-Toolchain baute Debug und Release strikt mit Tests, Beispielen und
Warnings-as-Errors. Jeweils 4/4 CTest-Ziele, fokussierte Core-/Numeric-Tests und beide Metadata-Demos
waren erfolgreich. Diff-, Link- und ADR-Prüfungen waren grün. Native-/Sanitizer-/SDL3-/GTK-/AppKit-
Prüfungen werden korrekt der CI zugeordnet; deren PR-/Merge-SHA-Ergebnisse stehen beim Pull Request.
