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

## Implementation and validation record

The implementation was delivered in commit `fafa59812cb48b0a0ec1f2198ecb587781052980` on branch
`codex/m8-runtime-type-identification` and published as PR #75. The final PR head remained this
commit. The change added the separate public runtime-metadata header and implementation, explicit
mapping for all 26 catalogued built-in types, focused exact-identity tests, the headless demo output,
and the ADR/roadmap/changelog documentation. The existing static type catalog and `Component` base
contract were not changed.

Strict GCC Debug and Release builds with C++20 and warnings as errors were run. The complete
configured CTest suite passed in both configurations (4/4 tests each), `git diff --check` passed,
and the headless metadata demo reported `sasd.ui.Button` and `sasd.ui.Label` as the exact runtime
types. The tests cover representative concrete built-ins and models, identity through a
`Component&`, unknown `Widget`/`Label`-derived types, catalog-pointer consistency and unchanged
ownership/parenting.

All required checks for PR #75 passed for the final head: Linux GCC, Linux Clang, Linux Clang
ASan + UBSan, Linux SDL3 adapter, Windows MSVC, macOS AppleClang, GTK4 Native, macOS AppKit Native,
Linux Release, Windows Release and macOS Release. PR #75 was merged by a normal merge commit as
`34361a424632f40842641af7856f8f31eec0de5a`.

Post-merge validation for exactly that merge SHA was fully green: Main CI including the complete
Linux, Windows and macOS Release matrix, GTK Native CI and AppKit Native CI. The feature branch was
deleted locally and remotely, followed by `fetch --prune`; local `main` was synchronized to the
merge SHA and the working tree was clean. PRs #30, #32 and #63 were not changed. No release or tag
was created. The visible macOS/Cocoa M3 gate remains open. M8 remains active; component description,
serialization, deserialization, factories, validation, design-time attributes, resources, designer
and IDE integration were not started.

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

### Implementierungs- und Validierungsprotokoll

Die Implementierung wurde im Commit `fafa59812cb48b0a0ec1f2198ecb587781052980` auf dem Branch
`codex/m8-runtime-type-identification` erstellt und als PR #75 veröffentlicht. Der finale PR-Head
blieb dieser Commit. Die Änderung ergänzte den separaten öffentlichen Runtime-Metadaten-Header und
seine Implementierung, die explizite Zuordnung aller 26 katalogisierten Built-in-Typen, fokussierte
Tests für exakte Identität, die Ausgabe im headless Demo sowie die ADR-/Roadmap-/Changelog-
Dokumentation. Der bestehende statische Typkatalog und der `Component`-Basisvertrag wurden nicht
verändert.

Es wurden strikte GCC-Debug- und Release-Builds mit C++20 und Warnings-as-Errors ausgeführt. Die
vollständige konfigurierte CTest-Suite war in beiden Konfigurationen erfolgreich (jeweils 4/4 Tests),
`git diff --check` war erfolgreich und das headless Metadata-Demo meldete `sasd.ui.Button` und
`sasd.ui.Label` als exakte Runtime-Typen. Die Tests prüfen repräsentative konkrete Built-ins und
Models, Identität durch eine `Component&`, unbekannte von `Widget`/`Label` abgeleitete Typen,
Katalog-Pointer-Konsistenz sowie unverändertes Ownership/Parenting.

Alle erforderlichen Checks von PR #75 waren für den finalen Head erfolgreich: Linux GCC, Linux Clang,
Linux Clang ASan + UBSan, Linux SDL3 Adapter, Windows MSVC, macOS AppleClang, GTK4 Native, macOS
AppKit Native, Linux Release, Windows Release und macOS Release. PR #75 wurde mit dem normalen
Merge-Commit `34361a424632f40842641af7856f8f31eec0de5a` gemergt.

Die Post-Merge-Validierung für genau diesen Merge-SHA war vollständig erfolgreich: Main CI
einschließlich der vollständigen Linux-/Windows-/macOS-Release-Matrix, GTK Native CI und AppKit
Native CI. Der Feature-Branch wurde lokal und remote gelöscht; anschließend wurde `fetch --prune`
ausgeführt, `main` auf den Merge-SHA synchronisiert und ein sauberer Working Tree bestätigt. Die
PRs #30, #32 und #63 wurden nicht verändert. Es wurde kein Release und kein Tag erzeugt. Das
sichtbare macOS/Cocoa-M3-Gate bleibt offen. M8 bleibt aktiv; Component Description, Serialisierung,
Deserialisierung, Factories, Validierung, Design-Time-Attribute, Ressourcen, Designer und
IDE-Integration wurden nicht begonnen.
