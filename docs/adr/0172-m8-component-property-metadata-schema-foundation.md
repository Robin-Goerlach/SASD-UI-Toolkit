# ADR 0172: M8 Component Property Metadata Schema Foundation

## Status

Accepted

## Predecessor

[0171](0171-m8-component-type-metadata-foundation.md)

## English

### Context

M8 Slice 1 established stable built-in Component type metadata. Developer tooling now needs to ask
which semantic properties a built-in type describes without constructing an instance or depending on
any backend. This slice must remain a schema foundation rather than silently becoming runtime
reflection, serialization or a designer contract.

### Decision

The Core exposes the separate `sasd/ui/component_property_metadata.hpp` header. Its immutable
`ComponentPropertyMetadata` descriptor contains:

- `declaring_type_id`, the explicit stable ID of the type that directly declares the property;
- `property_id`, a stable, non-localized, case-sensitive local semantic ID; and
- `value_kind`, currently limited to `boolean` and `string`.

The built-in catalog contains ten directly declared descriptors for `Widget`, `Command`, `Label`,
`Button`, `TextField` and `CheckBox`. `declaredBuiltInComponentProperties()` returns a read-only
span for one exact declaring type. `findBuiltInComponentProperty()` walks the existing type
metadata `base_type_id` chain and returns the original descriptor, so inherited properties are not
duplicated in derived type tables. Unknown types or properties fail closed with an empty span or
`nullptr`; identifiers are case-sensitive.

The first built-in catalog does not permit property shadowing along an ancestor chain. Sibling types
may use the same local ID, such as `text`, because the stable identity is initially the pair
`(declaring_type_id, property_id)`. The catalog is static, immutable, allocation-free for normal
queries and backend-neutral. Terminal remains a first-class consumer of the same Core boundary.

### Explicit boundaries

This slice does not read or write values on Component instances. It adds no virtual reflection API,
`std::variant`, `std::any`, `void*`, casts, getter/setter or member-function pointers, factory,
serialization, default values, validation attributes, display names, localization/editor metadata,
resource system or plugin registry. Third-party extension remains deferred until a concrete
extension contract exists.

### Consequences

Tooling can enumerate direct schema properties and resolve inherited schema properties without
creating Components or changing ownership. The value-kind vocabulary and the first property set are
intentionally small and can be extended by a later decision when a real consumer requires it.
Runtime value access, defaults, validation/design-time metadata and serialization must define their
own contracts in later M8 slices rather than expanding this schema descriptor spec speculatively.

## Deutsch

### Kontext

M8 Slice 1 schuf stabile Built-in-Komponenten-Typmetadaten. Developer-Tooling muss nun abfragen
können, welche semantischen Properties ein Built-in-Typ beschreibt, ohne eine Instanz zu erzeugen
oder von einem Backend abzuhängen. Dieser Slice bleibt deshalb eine Schema-Grundlage und wird nicht
unbemerkt zu Runtime-Reflection, Serialisierung oder einem Designer-Vertrag.

### Entscheidung

Der Core stellt den separaten Header `sasd/ui/component_property_metadata.hpp` bereit. Der
unveränderliche Deskriptor `ComponentPropertyMetadata` enthält:

- `declaring_type_id`, die explizite stabile ID des direkt deklarierenden Typs;
- `property_id`, eine stabile, nicht lokalisierte, case-sensitive lokale semantische ID; und
- `value_kind`, zunächst beschränkt auf `boolean` und `string`.

Der Built-in-Katalog enthält zehn direkt deklarierte Deskriptoren für `Widget`, `Command`, `Label`,
`Button`, `TextField` und `CheckBox`. `declaredBuiltInComponentProperties()` liefert einen
read-only-Span für einen exakten deklarierenden Typ. `findBuiltInComponentProperty()` läuft die
vorhandene `base_type_id`-Kette der Typmetadaten ab und liefert den ursprünglichen Deskriptor
zurück; geerbte Properties werden daher nicht in abgeleiteten Tabellen dupliziert. Unbekannte Typen
oder Properties führen fail-closed zu einem leeren Span bzw. `nullptr`; IDs sind case-sensitive.

Der erste Built-in-Katalog erlaubt kein Property-Shadowing entlang einer Ahnenkette. Sibling-Typen
dürfen dieselbe lokale ID wie `text` verwenden, weil die stabile Identität zunächst das Paar
`(declaring_type_id, property_id)` ist. Der Katalog ist statisch, unveränderlich, bei normalen
Abfragen allokationsfrei und backendneutral. Das Terminal bleibt ein First-Class-Consumer derselben
Core-Grenze.

### Ausdrückliche Grenzen

Dieser Slice liest oder schreibt keine Werte an Component-Instanzen. Er fügt keine virtuelle
Reflection-API, kein `std::variant`, `std::any`, `void*`, Casts, Getter-/Setter- oder Member-
Function-Pointer, Factory, Serialisierung, Default-Werte, Validierungsattribute, Anzeige- oder
Lokalisierungs-/Editor-Metadaten, kein Resource-System und keine Plugin-Registry hinzu. Drittanbieter-
Erweiterungen bleiben zurückgestellt, bis ein konkreter Erweiterungsvertrag vorliegt.

### Folgen

Tooling kann direkte Schema-Properties aufzählen und geerbte Schema-Properties auflösen, ohne
Components zu erzeugen oder Ownership zu verändern. Das ValueKind-Vokabular und der erste
Property-Satz sind bewusst klein und können durch eine spätere Entscheidung erweitert werden, wenn
ein realer Consumer dies benötigt. Runtime-Wertzugriff, Defaults, Validierung/Design-Time-Metadaten
und Serialisierung müssen in späteren M8-Slices eigene Verträge erhalten, statt diese Schema-
Deskriptor-Spezifikation spekulativ zu vergrößern.
