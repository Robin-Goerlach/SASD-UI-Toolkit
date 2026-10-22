# ADR 0175: M8 Exact Built-in Runtime Type Identification Foundation

- Status: Accepted
- Date: 2026-10-22

## Context

M8 already provides stable built-in type IDs, an immutable type catalog, property schema metadata,
typed runtime property access and effective property defaults. The next concrete consumer is expected
to be a component-description/serialization layer. That consumer must determine the exact built-in
type of an existing `const Component&`; accepting a caller-supplied base ID could make derived
properties such as `Button.text` disappear.

## Decision

Add the backend-neutral `builtInComponentTypeOf(const Component&) noexcept` query in a separate
runtime-metadata layer. It compares `typeid(component)` with every catalogued built-in C++ type and
returns the matching pointer from `builtInComponentTypes()` through `findBuiltInComponentType()`.
Unknown derived types return `nullptr`. There is no fallback to a known base type, no mutable
registry, no static-registration side effect, no third-party registration and no virtual `typeId()`
method on `Component`.

RTTI is an internal implementation detail only. `typeid(T).name()`, `hash_code()` and
`std::type_index` are not used as public or persisted identities. The stable toolkit `type_id` in
`ComponentTypeMetadata` remains the public identity. The query allocates nothing, creates no
components, initializes no backend and changes no ownership or component state.

## Consequences

Serialization can safely start from an existing component instance without silently reclassifying an
unknown derived object as a built-in base. The exact mapping must be kept in step with the immutable
26-entry built-in catalog. This slice does not implement component descriptions, serialization,
deserialization, factories, child enumeration, validation, design-time metadata, resources,
designer/IDE reflection or a persistence format.

## Deutsch

### Kontext

M8 stellt bereits stabile Built-in-Typ-IDs, einen unveränderlichen Typkatalog, Property-Schema-
Metadaten, typsicheren Runtime-Property-Zugriff und effektive Property-Defaults bereit. Der nächste
konkrete Consumer wird voraussichtlich eine Component-Description-/Serialisierungsschicht. Dieser
Consumer muss den exakten Built-in-Typ einer vorhandenen `const Component&` bestimmen; eine vom
Aufrufer behauptete Basis-ID könnte sonst abgeleitete Properties wie `Button.text` verlieren.

### Entscheidung

Die backendneutrale Abfrage `builtInComponentTypeOf(const Component&) noexcept` wird als separate
Runtime-Metadatenschicht ergänzt. Sie vergleicht `typeid(component)` mit jedem katalogisierten
Built-in-C++-Typ und liefert den passenden Pointer aus `builtInComponentTypes()` über
`findBuiltInComponentType()` zurück. Unbekannte Derived-Typen liefern `nullptr`; es gibt keinen
Fallback auf einen bekannten Basistyp, keine veränderliche Registry, keine Static-Registration-
Seiteneffekte, keine Drittanbieter-Registrierung und keine virtuelle `typeId()`-Methode in
`Component`.

RTTI bleibt ausschließlich internes Implementierungsdetail. `typeid(T).name()`, `hash_code()` und
`std::type_index` werden weder als öffentliche noch als persistierte Identität verwendet. Die stabile
Toolkit-`type_id` in `ComponentTypeMetadata` bleibt die öffentliche Identität. Die Abfrage alloziert
nicht, erzeugt keine Components, initialisiert kein Backend und verändert weder Ownership noch
Component-Zustand.

### Folgen

Serialisierung kann sicher von einer vorhandenen Component-Instanz ausgehen, ohne ein unbekanntes
abgeleitetes Objekt stillschweigend als Built-in-Basis umzudeuten. Die exakte Zuordnung muss mit dem
unveränderlichen 26-Einträge-Built-in-Katalog synchron bleiben. Dieser Slice implementiert noch keine
Component Description, Serialisierung, Deserialisierung, Factories, Child-Aufzählung, Validierung,
Design-Time-Metadaten, Ressourcen, Designer-/IDE-Reflection oder ein Persistenzformat.
